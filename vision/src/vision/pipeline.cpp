#include <unordered_set>
#include <future>
#include <thread>
#include <boost/thread/sync_queue.hpp>
#include "vision/pipeline.h"

namespace vision {

using namespace std;
using namespace cv;
using namespace cmn;

struct DetectTask {
    const Mat *image;
    promise<DetectedObjectList> p;

    DetectTask(const Mat* m) : image(m) {}
};

class DetectWorker {
public:
    DetectWorker(Detector *detector)
    : m_detector(detector) {
        m_thread = new thread([this] { work(); });
    }

    virtual ~DetectWorker() {
        m_queue.push(nullptr);
        m_thread->join();
        delete m_thread;
        delete m_detector;
    }

    future<DetectedObjectList> detect(const Mat *image) {
        auto task = new DetectTask(image);
        auto f = task->p.get_future();
        m_queue.push(task);
        return f;
    }

private:
    Detector* m_detector;
    boost::sync_queue<DetectTask*> m_queue;
    thread *m_thread;

    void work() {
        while (true) {
            DetectTask *task = m_queue.pull();
            if (task == nullptr) {
                break;
            }
            DetectResult result(m_detector, *task->image);
            task->p.set_value(result.objects);
            delete task;
        }
    }
};

PipelineModule::PipelineModule() {
}

PipelineModule::~PipelineModule() {
}

void PipelineModule::addFactory(const string& name,
                                DetectorFactory* factory) {
    m_factories[name] = factory;
}

struct FunctorFactory : DetectorFactory {
    function<Detector*()> fn;

    FunctorFactory(function<Detector*()> f) : fn(f) {}

    Detector* createDetector() {
        return fn();
    }
};

void PipelineModule::addFactory(const string& name,
                                function<Detector*()> factory) {
    addFactory(name, new FunctorFactory(factory));
}

DetectorFactory* PipelineModule::factoryFor(const string& name) const {
    auto it = m_factories.find(name);
    return it == m_factories.end() ? nullptr : it->second;
}

Detector* PipelineModule::createDetector(const string& name) const {
    auto f = factoryFor(name);
    if (f != nullptr) {
        return f->createDetector();
    }
    return nullptr;
}

void PipelineModule::initialize(Application* app) {
    m_app = app;
    app->options().add_options()
        ("detectors", "activate/deactivate detectors", cxxopts::value<vector<string>>())
    ;
    app->options().parse_positional(vector<string>{"detectors"});
}

void PipelineModule::start() {
    unordered_set<string> enabled;
    for (auto& name : m_app->opt("detectors").as<vector<string>>()) {
        if (!name.empty() && (name[0] == '~' || name[0] == '!')) {
            if (name.length() == 1) {
                for (auto& p : m_factories) {
                    enabled.clear();
                }
            } else {
                enabled.erase(name.substr(1));
            }
        } else if (name == "*") {
            for (auto& p : m_factories) {
                enabled.insert(p.first);
            }
        } else {
            enabled.insert(name);
        }
    }
    for (auto& p : m_factories) {
        if (enabled.find(p.first) != enabled.end()) {
            m_workers.push_back(new DetectWorker(p.second->createDetector()));
        }
    }
}

void PipelineModule::stop() {
    for (auto w : m_workers) {
        delete w;
    }
    m_workers.clear();
}

void PipelineModule::detect(const Mat &image, DetectedObjectList &objects) {
    list<future<DetectedObjectList>> futures;
    for (auto w : m_workers) {
        futures.push_back(w->detect(&image));
    }
    for (auto& f : futures) {
        objects.splice(objects.end(), f.get());
    }
}

}

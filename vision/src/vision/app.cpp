#include <iostream>
#include "vision/app.h"

namespace vision {

using namespace std;
using namespace boost::asio;
using namespace cv;
using namespace cmn;

BufferImage::BufferImage()
: m_owned(true) {
    m_data = new char[MAX_SIZE];
    m_size = MAX_SIZE;
}

BufferImage::BufferImage(const const_buffer& buf)
: m_owned(false),
  m_data(const_cast<char*>(buffer_cast<const char*>(buf))),
  m_size(buffer_size(buf)) {

}

BufferImage::BufferImage(const BufferImage& m, size_t sz)
: m_owned(false), m_data(m.m_data), m_size(sz) {

}

BufferImage::BufferImage(const BufferImage& m)
: m_owned(true) {
   m_data = new char[m.m_size];
   m_size = m.m_size;
   memcpy(m_data, m.m_data, m_size);
}

BufferImage::~BufferImage() {
    if (m_owned) {
        delete []m_data;
    }
}

bool BufferImage::decode(Mat& image) const {
    if (m_size == 0) {
        return false;
    }
    auto decoded = imdecode(Mat(1, m_size, CV_8UC1, m_data), IMREAD_UNCHANGED, &image);
    return !decoded.empty();
}

App::App(int argc, char** argv)
    : Application(argc, argv),
      m_captured(nullptr),
      m_pub(&m_mq) {
    m_mq.addFactory(MQTTAdapter::factory());
    addModule(&m_mq);
    m_pub.setHandler([this] (const Msg* msg) {
        imageCaptured(BufferImage(msg->raw()));
    });
    addModule(&m_pub);
    options().add_options()
        ("show", "show recognized area", cxxopts::value<bool>())
        ("quiet", "quiet, don't print result", cxxopts::value<bool>())
    ;
}

bool App::capture(Mat& image) {
    auto raw = m_captured.exchange(nullptr);
    if (raw == nullptr) {
        unique_lock<mutex> lock(m_captured_lock);
        m_captured_cond.wait(lock, [this]{ return m_captured.load() != nullptr; });
        raw = m_captured.exchange(nullptr);
    }
    bool decoded = false;
    if (raw != nullptr) {
        decoded = raw->decode(image);
        delete raw;
    }
    return decoded;
}

void App::imageCaptured(const RawImage& image) {
    auto existing = m_captured.exchange(image.clone());
    if (existing) {
        delete existing;
    }
    m_captured_cond.notify_all();
}

void App::finalizePipeline() {
    addModule(&m_pipeline);
}

void App::objectColor(const string& name, Scalar color) {
    m_colors[name] = color;
}

int App::run() {
    bool show = opt("show").as<bool>();
    bool quiet = opt("quiet").as<bool>();

    if (show) {
        namedWindow("vision");
    }

    Mat image;
    while (true) {
        if (!capture(image)) {
            if (show) {
                waitKey(1);
            }
            continue;
        }

        DetectResult result(&m_pipeline, image);
        if (!result.empty()) {
            if (!quiet) {
                cerr << result.json() << endl; cerr.flush();
            }
            m_pub.pub(result.json().dump());
        }
        if (show) {
            for (auto& obj : result.objects) {
                Scalar color(0, 0, 0);
                map<string, Scalar>::const_iterator it = m_colors.find(obj.type);
                if (it != m_colors.end()) {
                    color = it->second;
                }
                rectangle(image, obj.rc, color);
            }
            imshow("vision", image);
            waitKey(1);
        }
    }
    if (show) {
        waitKey();
    }
    return 0;
}

CaptureSource::CaptureSource()
: m_app(nullptr), m_stop(false), m_thread(nullptr) {

}

void CaptureSource::initialize(Application* app) {
    m_app = reinterpret_cast<App*>(app);
}

void CaptureSource::start() {
    m_thread = new thread([this] {
        while (!m_stop.load()) {
            auto frame = captureFrame();
            if (frame != nullptr) {
                m_app->imageCaptured(*frame);
                delete frame;
            }
        }
    });
}

void CaptureSource::stop() {
    m_stop.store(true);
    if (m_thread != nullptr) {
        m_thread->join();
    }
}

void CaptureSource::cleanup() {
    if (m_thread != nullptr) {
        delete m_thread;
        m_thread = nullptr;
    }
}

}

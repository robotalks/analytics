#pragma once

#include <list>
#include <unordered_map>
#include <memory>
#include <string>
#include <utility>
#include <functional>
#include "cmn/app.h"
#include "vision/vision.h"

namespace vision {

    struct DetectorFactory : ::cmn::Interface {
        virtual Detector* createDetector() = 0;
    };

    class DetectWorker;

    class PipelineModule : public ::cmn::AppModule, public Detector {
    public:
        PipelineModule();
        virtual ~PipelineModule();

        void addFactory(const ::std::string& name, DetectorFactory*);
        void addFactory(const ::std::string& name, ::std::function<Detector*()>);

        DetectorFactory* factoryFor(const ::std::string& name) const;
        Detector* createDetector(const ::std::string& name) const;

        void initialize(::cmn::Application*);
        void start();
        void stop();
        void cleanup() {}

        void detect(const ::cv::Mat &image, DetectedObjectList &objects);

        ::cmn::Application* app() const { return m_app; }

    protected:
        ::std::unordered_map<::std::string, DetectorFactory*> m_factories;
        ::std::list<DetectWorker*> m_workers;

    private:
        ::cmn::Application* m_app;
    };

}

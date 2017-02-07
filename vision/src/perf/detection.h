#pragma once

#include <map>
#include <string>
#include <vector>
#include "cmn/app.h"
#include "vision/vision.h"

class Detection {
public:
    Detection();

    void init(::cmn::Application*);
    ::vision::Detector* get(const ::std::string& name) const;
    ::std::vector<::std::string> names() const;

private:
    ::std::map<::std::string, ::vision::Detector*> m_detectors;
};

class DetectApp : public ::cmn::Application {
public:
    DetectApp(int argc, char **argv)
    : ::cmn::Application(argc, argv) {
        m_detection.init(this);
    }

    ::std::vector<::std::string> detectors() const {
        return m_detection.names();
    }

    ::vision::Detector* detector(const ::std::string& name) const {
        return m_detection.get(name);
    }

private:
    Detection m_detection;
};

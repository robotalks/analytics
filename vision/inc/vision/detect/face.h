#pragma once

#include <string>
#include "vision/app.h"

namespace vision {

class FaceDetector : public Detector {
public:
    FaceDetector(const ::std::string& dir, bool lbp, double scaleFactor, int minNeighbors, int minSize);

    void detect(const ::cv::Mat& image, DetectedObjectList& objects);

    static void reg(App*, const ::std::string& name = "face");
    static void reg(App*, ::cv::Scalar color, const ::std::string& name = "face");

private:
    ClassifyModel m_frontalface;
    ClassifyModel m_eyes;
};

class FaceDetectorFactory : public DetectorFactory {
public:
    FaceDetectorFactory(::cmn::Application *app);
    Detector* createDetector();

private:
    ::cmn::Application *m_app;
};

}

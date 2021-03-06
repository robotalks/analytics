#pragma once

#include <string>
#include "vision/app.h"

namespace vision {

class SimpleMotionDetector : public Detector {
public:
    SimpleMotionDetector(const ::cv::Size& blurSize,
        double areaMin = 0, double threshold = 25);

    void detect(const ::cv::Mat& image, DetectedObjectList& objects);

    static void reg(App*, const ::std::string& name = "motion-simple");
    static void reg(App*, ::cv::Scalar color, const ::std::string& name = "motion-simple");

private:
    ::cv::Size m_blurSize;
    double m_areaMin;
    double m_threshold;
    ::cv::Mat m_prev;
};

}

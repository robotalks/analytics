#pragma once

#include <vector>
#include <string>
#include <memory>
#include <opencv2/tracking/tracker.hpp>
#include "vision/app.h"

namespace vision {

class ObjectTracker : public Detector {
public:
    ObjectTracker(::std::shared_ptr<Detector> objDetector);

    void detect(const ::cv::Mat& image, DetectedObjectList& objects);

    static void reg(App*, const ::std::string& name = "track");
    static void reg(App*, ::cv::Scalar color, const ::std::string& name = "track");

private:
    ::std::shared_ptr<Detector> m_detector;

    struct Object {
        DetectedObject o;
        ::cv::Rect2d rc;
        ::cv::Ptr<::cv::Tracker> tracker;

        Object(const DetectedObject& obj) : o(obj), rc(obj.rc) {}
    };
    ::std::list<Object> m_objs;
};

}

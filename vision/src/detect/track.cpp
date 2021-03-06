#include "vision/detect/face.h"
#include "vision/detect/track.h"

namespace vision {

using namespace std;
using namespace cv;

ObjectTracker::ObjectTracker(shared_ptr<Detector> objDetector)
: m_detector(objDetector) {
}

void ObjectTracker::detect(const Mat& image, DetectedObjectList& objects) {
    if (m_objs.empty()) {
        DetectedObjectList objs;
        m_detector->detect(image, objs);
        for (auto& o : objs) {
            Object obj(o);
            obj.tracker = TrackerKCF::create();
            if (obj.tracker == nullptr) {
                throw runtime_error("create KCF tracker");
            }
            obj.tracker->init(image, obj.rc);
            m_objs.push_back(obj);
        }
        objects.splice(objects.end(), objs);
    } else {
        Rect2d b(0, 0, image.cols, image.rows);
        for (auto it = m_objs.begin(); it != m_objs.end(); ) {
            Object& o = *it;
            auto pos = it;
            it ++;
            if (o.tracker->update(image, o.rc) && (b & o.rc).area() > 0) {
                objects.push_back(DetectedObject(o.rc, o.o.type));
            } else {
                m_objs.erase(pos);
            }
        }
    }
}

void ObjectTracker::reg(App* app, const string& name) {
    app->options().add_options()
        ("track-detector", "detector for initial detection (face)", cxxopts::value<string>())
    ;
    app->pipeline()->addFactory(name, [app] {
        auto detectorName = app->opt("track-detector").as<string>();
        if (detectorName.empty()) {
            detectorName = "face";
        }
        Detector* detector = app->pipeline()->createDetector(detectorName);
        if (!detector) {
            throw invalid_argument("track-detector: " + detectorName);
        }
        return new ObjectTracker(shared_ptr<Detector>(detector));
    });
}

void ObjectTracker::reg(App* app, Scalar color, const string& name) {
    reg(app, name);
    app->objectColor("track", color);
}

}

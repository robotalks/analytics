#include <vector>
#include "vision/detect/motion.h"

namespace vision {

using namespace std;
using namespace cv;

SimpleMotionDetector::SimpleMotionDetector(const Size &blurSize, double areaMin)
: m_blurSize(blurSize), m_areaMin(areaMin) {

}

void SimpleMotionDetector::detect(const Mat& image, DetectedObjectList& objects) {
    Mat gray;
    cvtColor(image, gray, CV_BGR2GRAY);
    GaussianBlur(gray, gray, m_blurSize, 0);
    if (m_prev.empty()) {
        m_prev = gray;
        return;
    }

    Mat thresh;
    absdiff(m_prev, gray, thresh);
    m_prev = gray;
	threshold(thresh, thresh, 25, 255, THRESH_BINARY);
	dilate(thresh, thresh, Mat());
    vector<vector<Point>> contours;
    findContours(thresh.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	for (auto& c : contours) {
		if (contourArea(c) >= m_areaMin) {
		    objects.push_back(DetectedObject(boundingRect(c), "motion"));
        }
    }
}

void SimpleMotionDetector::reg(App* app, const ::std::string& name) {
    app->options().add_options()
        ("motion-min-area", "minimum motion area", cxxopts::value<double>())
        ("motion-blur", "size of gaussian blur", cxxopts::value<int>())
    ;
    app->pipeline()->addFactory(name, [app] {
        auto blurSize = app->opt("motion-blur").as<int>();
        if (blurSize <= 0) {
            blurSize = 11;
        }
        return new SimpleMotionDetector(
            Size(blurSize, blurSize),
            app->opt("motion-min-area").as<double>());
    });
}

void SimpleMotionDetector::reg(App* app, ::cv::Scalar color, const ::std::string& name) {
    reg(app, name);
    app->objectColor("motion", color);
}

}

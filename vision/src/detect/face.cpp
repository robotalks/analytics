#include "vision/app.h"
#include "vision/detect/face.h"

namespace vision {

using namespace std;
using namespace cv;

FaceDetector::FaceDetector(const string& dir, bool lbp, double scaleFactor, int minNeighbors, int minSize)
: m_frontalface("face", dir + (lbp ? "lbpcascades/lbpcascade_frontalface.xml" : "haarcascades/haarcascade_frontalface_default.xml")),
  m_eyes("eyes", dir + "haarcascades/haarcascade_eye_tree_eyeglasses.xml") {
      m_frontalface.scaleFactor(scaleFactor);
      m_frontalface.minNeighbors(minNeighbors);
      m_frontalface.minSize(Size(minSize, minSize));
}

size_t FaceDetector::detect(const Mat& image, DetectedObjectList& objects) {
    size_t count = 0;
    DetectResult result(&m_frontalface, image);
    for (auto& obj : result.objects) {
        //cout << obj.json() << endl;
        auto subImg = image(obj.rc);
        if (!DetectResult(&m_eyes, subImg).empty()) {
            objects.push_back(obj);
            count ++;
        }
    }
    return count;
}

void FaceDetector::reg(App* app, const ::std::string& name) {
    app->options().add_options()
        ("face-lbp", "use lbp detector", cxxopts::value<bool>())
        ("face-model-dir", "detection models dir", cxxopts::value<string>())
        ("face-min-size", "minimum face size", cxxopts::value<int>())
        ("face-min-neighbors", "minimum neighbors", cxxopts::value<int>())
        ("face-scale", "scale factor", cxxopts::value<double>())
    ;
    app->pipeline()->addFactory(name, [app] {
        auto modelDir = app->opt("face-model-dir").as<string>();
        if (modelDir.empty()) {
            modelDir = app->exeDir() + "/../share/OpenCV/";
        }
        auto minSize = app->opt("face-min-size").as<int>();
        if (minSize <= 0) {
            minSize = 60;
        }
        auto minNeighbors = app->opt("face-min-neighbors").as<int>();
        if (minNeighbors <= 0) {
            minNeighbors = 1;
        }
        auto scaleFactor = app->opt("face-scale").as<double>();
        if (scaleFactor <= 0) {
            scaleFactor = 2;
        }
        return new FaceDetector(modelDir,
            app->opt("face-lbp").as<bool>(),
            scaleFactor, minNeighbors, minSize);
    });
}

void FaceDetector::reg(App* app, ::cv::Scalar color, const ::std::string& name) {
    reg(app, name);
    app->objectColor("face", color);
}

}

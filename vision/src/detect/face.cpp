#include "vision/app.h"
#include "vision/detect/face.h"

namespace vision {

using namespace std;
using namespace cv;
using namespace cmn;

FaceDetector::FaceDetector(const string& dir, bool lbp, double scaleFactor, int minNeighbors, int minSize)
: m_frontalface("face", dir + (lbp ? "lbpcascades/lbpcascade_frontalface.xml" : "haarcascades/haarcascade_frontalface_default.xml")),
  m_eyes("eyes", dir + "haarcascades/haarcascade_eye_tree_eyeglasses.xml") {
      m_frontalface.scaleFactor(scaleFactor);
      m_frontalface.minNeighbors(minNeighbors);
      m_frontalface.minSize(Size(minSize, minSize));
}

void FaceDetector::detect(const Mat& image, DetectedObjectList& objects) {
    DetectResult result(&m_frontalface, image);
    for (auto& obj : result.objects) {
        auto subImg = image(obj.rc);
        if (!DetectResult(&m_eyes, subImg).empty()) {
            objects.push_back(obj);
        }
    }
}

void FaceDetector::reg(App* app, const string& name) {
    app->options().add_options()
        ("face-lbp", "use lbp detector", cxxopts::value<bool>())
        ("face-model-dir", "detection models dir", cxxopts::value<string>())
        ("face-min-size", "minimum face size", cxxopts::value<int>())
        ("face-min-neighbors", "minimum neighbors", cxxopts::value<int>())
        ("face-scale", "scale factor", cxxopts::value<double>())
    ;
    app->pipeline()->addFactory(name, new FaceDetectorFactory(app));
}

void FaceDetector::reg(App* app, Scalar color, const string& name) {
    reg(app, name);
    app->objectColor("face", color);
}

FaceDetectorFactory::FaceDetectorFactory(Application *app)
: m_app(app) {
}

Detector* FaceDetectorFactory::createDetector() {
    auto modelDir = m_app->opt("face-model-dir").as<string>();
    if (modelDir.empty()) {
        modelDir = m_app->exeDir() + "/../share/OpenCV/";
    }
    auto minSize = m_app->opt("face-min-size").as<int>();
    if (minSize <= 0) {
        minSize = 60;
    }
    auto minNeighbors = m_app->opt("face-min-neighbors").as<int>();
    if (minNeighbors <= 0) {
        minNeighbors = 1;
    }
    auto scaleFactor = m_app->opt("face-scale").as<double>();
    if (scaleFactor <= 0) {
        scaleFactor = 2;
    }
    return new FaceDetector(modelDir,
        m_app->opt("face-lbp").as<bool>(),
        scaleFactor, minNeighbors, minSize);
}

}

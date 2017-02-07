#include "vision/app.h"
#include "vision/detect.h"

using namespace std;
using namespace boost::asio;
using namespace cv;
using namespace cmn;
using namespace vision;

class App : public vision::App {
public:
    App(int argc, char** argv)
        : vision::App(argc, argv) {

        addModule(new UDPCastCapture());
        addModule(new OpenCVCapture());

        FaceDetector::reg(this, Scalar(255, 0, 0));
        SimpleMotionDetector::reg(this, Scalar(0, 0, 255));
        finalizePipeline();

        objectColor("smile", Scalar(0, 255, 0));
    }
};

int main(int argc, char* argv[]) {
    return ::App(argc, argv).main();
}

#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

int main() {
    cv::VideoCapture capl(1), capr(2);
    capl.set(CV_CAP_PROP_FRAME_WIDTH, 320);
    capl.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
    //capl.set(CV_CAP_PROP_FPS, 15);
    capr.set(CV_CAP_PROP_FRAME_WIDTH, 320);
    capr.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
    //capr.set(CV_CAP_PROP_FPS, 15);

    cv::Mat imgl, imgr;
    while (true) {
        bool grabl = false, grabr = false;
        while (!grabl || !grabr) {
            if (!grabl) grabl = capl.read(imgl);
            if (!grabr) grabr = capr.read(imgr);
            if (grabl && grabr) break;
            usleep(1000);
        }
        cv::imshow("capl", imgl);
        cv::imshow("capr", imgr);
        if (cv::waitKey(1) == 27) {
            break;
        }
    }
}

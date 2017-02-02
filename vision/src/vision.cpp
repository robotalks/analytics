#include <iostream>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <boost/asio/buffer.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "cmn/app.h"
#include "cmn/pubsub.h"
#include "cmn/mqtt.h"
#include "cmn/perflog.h"
#include "vision/pipeline.h"
#include "face_detector.h"

using namespace std;
using namespace boost::asio;
using namespace cv;
using namespace cmn;
using namespace vision;

class App : public cmn::Application {
public:
    App(int argc, char** argv)
        : cmn::Application(argc, argv),
          m_pub(&m_mq) {
        m_msg = nullptr;
        m_mq.addFactory(MQTTAdapter::factory());
        addModule(&m_mq);
        m_pub.setHandler(std::bind(&App::onMessage, this, std::placeholders::_1));
        addModule(&m_pub);
        m_pipeline.addFactory("face", [this] {
            return new FaceDetector(exeDir() + "/../share/OpenCV/", opt("lbp").as<bool>(), 2, 1, 60);
        });
        addModule(&m_pipeline);
        options().add_options()
            ("lbp", "use lbp detector", cxxopts::value<bool>())
            ("show", "show recognized area", cxxopts::value<bool>())
            ("quiet", "quiet, don't print result", cxxopts::value<bool>())
        ;
    }

protected:
    virtual int run() {
        bool show = opt("show").as<bool>();
        bool quiet = opt("quiet").as<bool>();

        if (show) {
            namedWindow("vision");
        }

        Mat image;
        while (true) {
            if (!capture(image)) {
                if (show) {
                    waitKey(1);
                }
                continue;
            }

            DetectResult result(&m_pipeline, image);
            if (!result.empty()) {
                if (!quiet) {
                    cerr << result.json() << endl; cerr.flush();
                }
                m_pub.pub(result.json().dump());
            }
            if (show) {
                for (auto& obj : result.objects) {
                    rectangle(image, obj.rc, obj.type == "smile" ? Scalar(0, 255, 0) : Scalar(255, 0, 0));
                }
                imshow("vision", image);
                waitKey(1);
            }
        }
        if (show) {
            waitKey();
        }
        return 0;
    }

private:
    MQConnector m_mq;
    PubSub m_pub;
    PipelineModule m_pipeline;

    atomic<Msg*> m_msg;
    mutex m_msg_lock;
    condition_variable m_msg_cond;

    bool capture(Mat& image) {
        auto msg = m_msg.exchange(nullptr);
        if (msg == nullptr) {
            return false;
            unique_lock<mutex> lock(m_msg_lock);
            m_msg_cond.wait(lock, [this]{ return m_msg.load() != nullptr; });
            msg = m_msg.exchange(nullptr);
        }
        bool decoded = false;
        if (msg != nullptr) {
            decoded = decodeImage(msg->raw(), image);
            delete msg;
        }
        return decoded;
    }

    bool decodeImage(const const_buffer& buf, Mat& image) {
        auto size = buffer_size(buf);
        if (size == 0) {
            return false;
        }
        auto ptr = buffer_cast<const unsigned char*>(buf);
        auto decoded = imdecode(Mat(1, size, CV_8UC1, const_cast<unsigned char*>(ptr)), IMREAD_UNCHANGED, &image);
        return !decoded.empty();
    }

    void onMessage(const Msg* msg) {
        auto existing = m_msg.exchange(msg->copy());
        if (existing) {
            delete existing;
        }
        m_msg_cond.notify_all();
    }
};

int main(int argc, char* argv[]) {
    return App(argc, argv).main();
}

#include <iostream>
#include <chrono>
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <cstdlib>
#include <condition_variable>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
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
          m_pub(&m_mq),
          m_udpsock(m_io) {
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
            ("udp", "capture image using UDP", cxxopts::value<string>())
            ("lbp", "use lbp detector", cxxopts::value<bool>())
            ("show", "show recognized area", cxxopts::value<bool>())
            ("quiet", "quiet, don't print result", cxxopts::value<bool>())
        ;
    }

protected:
    virtual int run() {
        startUDPCapture(opt("udp").as<string>());

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
    io_service m_io;
    ip::udp::socket m_udpsock;

    atomic<Msg*> m_msg;
    mutex m_msg_lock;
    condition_variable m_msg_cond;

    class UDPMsg : public Msg {
    public:
        UDPMsg() : m_owned(true) {
            m_data = new char[MAX_IMAGE_SIZE];
            m_size = MAX_IMAGE_SIZE;
        }

        UDPMsg(const UDPMsg& m, size_t sz)
        : m_owned(false), m_data(m.m_data), m_size(sz) {

        }

        virtual ~UDPMsg() {
            if (m_owned) {
                delete []m_data;
            }
        }

        mutable_buffers_1 recvBuf() { return buffer(m_data, m_size); }

        virtual const string& topic() const { return m_topic; }
        virtual const_buffer raw() const { return const_buffer(m_data, m_size); }
        virtual Msg* copy() const { return new UDPMsg(*this); }

    protected:
        UDPMsg(const UDPMsg& m) : m_owned(true) {
            m_data = new char[m.m_size];
            m_size = m.m_size;
            memcpy(m_data, m.m_data, m_size);
        }

    private:
        string m_topic;
        char *m_data;
        size_t m_size;
        bool m_owned;

        const size_t MAX_IMAGE_SIZE = 4096*1024;
    };

    void startUDPCapture(const string& addr) {
        if (addr.empty()) {
            return;
        }

        auto pos = addr.find_first_of(":");
        if (pos == string::npos) {
            throw invalid_argument("udp address: port unspecified");
        }
        int port = atoi(addr.substr(pos+1).c_str());
        if (pos <= 0) {
            throw invalid_argument("udp address: invalid port");
        }
        boost::system::error_code ec;
        ip::address ipaddr = ip::address::from_string(addr.substr(0, pos), ec);
        if (ec) {
            throw invalid_argument("udp_address: " + ec.message());
        }
        ip::udp::endpoint endpoint(ipaddr, port);
        m_udpsock.open(endpoint.protocol());
        m_udpsock.set_option(ip::udp::socket::reuse_address(true));
        if (ipaddr.is_multicast()) {
            m_udpsock.bind(ip::udp::endpoint(ip::address_v4::any(), port));
            m_udpsock.set_option(ip::multicast::join_group(ipaddr));
        } else {
            m_udpsock.bind(endpoint);
        }

        thread t([this] { recvFromUDP(); });
        t.detach();
    }

    void recvFromUDP() {
        UDPMsg msg;
        ip::udp::endpoint from;
        while (true) {
            size_t sz = m_udpsock.receive_from(msg.recvBuf(), from);
            UDPMsg m(msg, sz);
            onMessage(&m);
        }
    }

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

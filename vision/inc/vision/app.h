#pragma once

#include <atomic>
#include <mutex>
#include <cstdlib>
#include <condition_variable>
#include <thread>
#include <map>
#include <string>
#include <opencv2/highgui/highgui.hpp>
#include <boost/asio.hpp>
#include "cmn/pubsub.h"
#include "cmn/mqtt.h"
#include "vision/pipeline.h"

namespace vision {

struct RawImage : ::cmn::Interface {
    virtual RawImage* clone() const = 0;
    virtual bool decode(::cv::Mat& output) const = 0;
};

class BufferImage : public RawImage {
public:
    const size_t MAX_SIZE = 4096*1024;

    BufferImage();

    // a slice reference to data
    BufferImage(const ::boost::asio::const_buffer&);

    // a slice reference to the data
    BufferImage(const BufferImage&, size_t);

    virtual ~BufferImage();

    // expose the buffer for receiving data
    ::boost::asio::mutable_buffers_1 recvBuf() {
        return ::boost::asio::buffer(m_data, m_size);
    }

    virtual bool decode(::cv::Mat&) const;
    virtual RawImage* clone() const { return new BufferImage(*this); }

protected:
    // copy of data
    BufferImage(const BufferImage& m);

private:
    char *m_data;
    size_t m_size;
    bool m_owned;
};

class App : public ::cmn::Application {
public:
    App(int argc, char** argv);

    bool capture(::cv::Mat&);
    void imageCaptured(const RawImage&);

    PipelineModule* pipeline() { return &m_pipeline; }
    void finalizePipeline();

    void objectColor(const ::std::string& name, ::cv::Scalar color);

protected:
    virtual int run();

private:
    ::cmn::MQConnector m_mq;
    ::cmn::PubSub m_pub;
    PipelineModule m_pipeline;

    ::std::map<::std::string, ::cv::Scalar> m_colors;

    ::std::atomic<RawImage*> m_captured;
    ::std::mutex m_captured_lock;
    ::std::condition_variable m_captured_cond;
};

class CaptureSource : public ::cmn::AppModule {
public:
    virtual void initialize(::cmn::Application*);
    virtual void start();
    virtual void stop();
    virtual void cleanup();

    App* app() const { return m_app; }

protected:
    CaptureSource();

    virtual RawImage* captureFrame() { return nullptr; }

private:
    App *m_app;
    ::std::atomic_bool m_stop;
    ::std::thread *m_thread;
};

class UDPCastCapture : public CaptureSource {
public:
    UDPCastCapture();

    void initialize(::cmn::Application*);
    void start();

protected:
    RawImage* captureFrame();

private:
    ::boost::asio::io_service m_io;
    ::boost::asio::ip::udp::socket m_socket;
    BufferImage m_buf;
};

class OpenCVCapture : public CaptureSource {
public:
    OpenCVCapture();

    void initialize(::cmn::Application*);
    void start();
    void cleanup();

protected:
    RawImage* captureFrame();

private:
    ::cv::VideoCapture *m_cap;
    ::cv::Mat m_image;
};

}

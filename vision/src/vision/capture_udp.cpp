#include <boost/system/error_code.hpp>
#include "vision/app.h"

namespace vision {
    using namespace std;
    using namespace boost::asio;
    using namespace cmn;

    UDPCastCapture::UDPCastCapture()
    : m_socket(m_io) {
    }

    void UDPCastCapture::initialize(Application *app) {
        CaptureSource::initialize(app);
        app->options().add_options()
            ("udp", "capture image using UDP", cxxopts::value<string>())
        ;
    }

    void UDPCastCapture::start() {
        auto addr = app()->opt("udp").as<string>();
        if (addr.empty()) {
            return;
        }

        auto pos = addr.find_first_of(":");
        if (pos == string::npos) {
            throw invalid_argument("udp address: port unspecified");
        }
        int port = stoi(addr.substr(pos+1));
        if (pos <= 0) {
            throw invalid_argument("udp address: invalid port");
        }
        boost::system::error_code ec;
        ip::address ipaddr = ip::address::from_string(addr.substr(0, pos), ec);
        if (ec) {
            throw invalid_argument("udp_address: " + ec.message());
        }
        ip::udp::endpoint endpoint(ipaddr, port);
        m_socket.open(endpoint.protocol());
        m_socket.set_option(ip::udp::socket::reuse_address(true));
        if (ipaddr.is_multicast()) {
            m_socket.bind(ip::udp::endpoint(ip::address_v4::any(), port));
            m_socket.set_option(ip::multicast::join_group(ipaddr));
        } else {
            m_socket.bind(endpoint);
        }
        CaptureSource::start();
    }

    RawImage* UDPCastCapture::captureFrame() {
        ip::udp::endpoint from;
        size_t sz = m_socket.receive_from(m_buf.recvBuf(), from);
        if (sz > 0) {
            return new BufferImage(m_buf, sz);
        }
        return nullptr;
    }
}

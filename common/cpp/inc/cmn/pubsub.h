#pragma once

#include <string>
#include <list>
#include <future>
#include <functional>
#include <boost/asio/buffer.hpp>
#include <boost/network/uri.hpp>
#include "cmn/common.h"
#include "cmn/app.h"

namespace cmn {
    struct PubOptions {
        bool retain;

        PubOptions() : retain(false) {}

        PubOptions& setRetain(bool retain) {
            this->retain = retain;
            return *this;
        }
    };

    class Msg;

    typedef ::std::function<void(const Msg*)> MsgHandler;

    struct MQAdapter : Interface {
        virtual ::std::future<void> connect() = 0;
        virtual ::std::future<void> disconnect() = 0;
        virtual ::std::future<void> sub(const ::std::string& topic, MsgHandler handler) = 0;
        virtual ::std::future<void> unsub(const ::std::string& topic) = 0;
        virtual ::std::future<void> pub(const ::std::string& topic,
            const ::boost::asio::const_buffer& data,
            const PubOptions&) = 0;
    };

    struct MQAdapterFactory : Interface {
        virtual MQAdapter* createAdapter(const ::boost::network::uri::uri& uri) = 0;
    };

    struct Msg : Interface {
        virtual const ::std::string& topic() const = 0;
        virtual ::boost::asio::const_buffer raw() const = 0;
        virtual Msg* copy() const = 0;
    };

    class MQConnector : public AppModule {
    public:
        MQConnector();
        virtual ~MQConnector();

        void addFactory(MQAdapterFactory*);

        void initialize(Application*);
        void start();
        void stop();
        void cleanup();

        ::std::future<void> sub(const ::std::string& topic, MsgHandler handler);
        ::std::future<void> unsub(const ::std::string& topic);
        ::std::future<void> pub(const ::std::string& topic,
            const ::boost::asio::const_buffer& data,
            const PubOptions&);

    private:
        Application* m_app;
        ::std::list<MQAdapterFactory*> m_factories;
        MQAdapter* m_adapter;
    };

    class PubSub : public AppModule {
    public:
        PubSub(MQConnector *connector = nullptr,
            const ::std::string& subTopicOpt = "sub",
            const ::std::string& pubTopicOpt = "pub");

        void initialize(Application*);
        void start();
        void stop();
        void cleanup();

        void setConnector(MQConnector *conn) { m_conn = conn; }
        MQConnector* getConnector() const { return m_conn; }

        void setHandler(MsgHandler handler) { m_handler = handler; }
        MsgHandler getHandler() const { return m_handler; }

        ::std::future<void> pub(const ::boost::asio::const_buffer& data, const PubOptions& opts = PubOptions());
        ::std::future<void> pub(const ::std::string& subTopic,
            const ::boost::asio::const_buffer& data, const PubOptions& opts = PubOptions());

        ::std::future<void> pub(const ::std::string& str, const PubOptions& opts = PubOptions()) {
            return pub(::boost::asio::const_buffer(str.c_str(), str.length()), opts);
        }

        ::std::future<void> pub(const ::std::string& subTopic,
            const ::std::string& str, const PubOptions& opts = PubOptions()) {
            return pub(subTopic, ::boost::asio::const_buffer(str.c_str(), str.length()), opts);
        }

    private:
        MQConnector* m_conn;
        ::std::string m_subOpt, m_pubOpt;
        Application* m_app;
        ::std::string m_sub, m_pub;
        MsgHandler m_handler;
    };
}

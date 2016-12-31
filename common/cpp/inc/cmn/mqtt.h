#pragma once

#include <stdexcept>
#include <future>
#include <unordered_map>
#include <mutex>
#include <boost/network/uri.hpp>
#include <mosquitto.h>
#include "cmn/pubsub.h"

namespace cmn {
    class MQTTError : public ::std::runtime_error {
    public:
        MQTTError(const ::std::string& msg);
        MQTTError(int rc, const ::std::string& msg);

        int code() const { return m_rc; }

    private:
        int m_rc;
    };

    class MQTTMsg : public Msg {
    public:
        ~MQTTMsg();

        const ::std::string& topic() const { return m_topic; }
        ::boost::asio::const_buffer raw() const;
        Msg* copy() const;

    protected:
        MQTTMsg(const struct mosquitto_message* msg);
        MQTTMsg(const MQTTMsg&);

    private:
        const struct mosquitto_message* m_msg;
        ::std::string m_topic;
        bool m_owned;

        friend class MQTTAdapter;
    };

    class MQTTAdapter : public MQAdapter {
    public:
        MQTTAdapter(const ::std::string& clientId = ::std::string(""));
        ~MQTTAdapter();

        void setHost(const ::std::string& host) { m_host = host; }
        const ::std::string& getHost() const { return m_host; }

        void setPort(int port) { m_port = port; }
        int getPort() const { return m_port; }

        void setServer(const ::std::string& host, int port) {
            setHost(host);
            setPort(port);
        }

        void setKeepAlive(int keepalive) { m_keepalive = keepalive; }
        int getKeepAlive() const { return m_keepalive; }

        ::std::future<void> connect();
        ::std::future<void> disconnect();
        ::std::future<void> sub(const ::std::string& topic, MsgHandler handler);
        ::std::future<void> unsub(const ::std::string& topic);
        ::std::future<void> pub(const ::std::string& topic,
            const ::boost::asio::const_buffer& data,
            const PubOptions&);

        static MQAdapterFactory* factory();

    private:
        ::std::string m_clientId;
        ::std::string m_host;
        int m_port;
        int m_keepalive;
        struct mosquitto *m_mosquitto;
        ::std::unordered_map<::std::string, ::std::list<MsgHandler> > m_subs;
        ::std::mutex m_subsLock;
        ::std::unordered_map<int, ::std::promise<void>*> m_subOps;
        ::std::mutex m_subOpsLock;
        ::std::unordered_map<int, ::std::promise<void>*> m_pubOps;
        ::std::mutex m_pubOpsLock;
        volatile ::std::promise<void>* m_connOp;
        volatile ::std::promise<void>* m_disconnOp;
        volatile bool m_loop;
        ::std::mutex m_connLock;

        void onConnect(int);
        void onDisconnect(int);
        void onSubscribe(int, int, const int*);
        void onUnsubscribe(int);
        void onPublish(int);
        void onMessage(const struct mosquitto_message*);

        friend class MQTTCallbacks;
    };
}

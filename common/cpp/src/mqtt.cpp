#include <cstdlib>
#include <sstream>
#include <functional>
#include <exception>
#include <iostream>
#include "cmn/mqtt.h"

namespace cmn {
    using namespace std;
    using namespace boost::asio;
    using namespace boost::network::uri;

    string format_errmsg(const string& msg, int rc) {
        ostringstream os;
        os << msg << ": " << rc;
        return os.str();
    }

    MQTTError::MQTTError(const string& msg)
    : runtime_error(msg), m_rc(0) {
    }

    MQTTError::MQTTError(int rc, const string& msg)
    : runtime_error(format_errmsg(msg, rc)), m_rc(rc) {
    }

    MQTTMsg::MQTTMsg(const struct mosquitto_message *msg)
    : m_msg(msg), m_topic(msg->topic), m_owned(false) {
    }

    MQTTMsg::MQTTMsg(const MQTTMsg& src)
    : m_topic(src.m_topic), m_owned(true) {
        auto dst = new struct mosquitto_message;
        int rc = mosquitto_message_copy(dst, src.m_msg);
        if (rc != MOSQ_ERR_SUCCESS) {
            throw MQTTError(rc, "copy message");
        }
        m_msg = dst;
    }

    MQTTMsg::~MQTTMsg() {
        if (m_owned) {
            auto dst = const_cast<struct mosquitto_message*>(m_msg);
            mosquitto_message_free(&dst);
        }
    }

    const_buffer MQTTMsg::raw() const {
        return const_buffer(m_msg->payload, m_msg->payloadlen);
    }

    Msg* MQTTMsg::copy() const {
        return new MQTTMsg(*this);
    }

    promise<void>* promise_or_throw(int rc, const string& errmsg) {
        if (rc != MOSQ_ERR_SUCCESS) {
            throw MQTTError(rc, errmsg);
        }
        return new promise<void>();
    }

    void complete_promise(promise<void>* p, int rc, const string& errmsg) {
        if (p) {
            if (rc == MOSQ_ERR_SUCCESS) {
                p->set_value();
            } else {
                p->set_exception(make_exception_ptr(MQTTError(rc, errmsg)));
            }
            delete p;
        }
    }

    void complete_op(mutex& m, unordered_map<int, promise<void>*>& ops, int mid) {
        lock_guard<mutex> g(m);
        auto it = ops.find(mid);
        if (it != ops.end()) {
            it->second->set_value();
            ops.erase(it);
        }
    }

    class MQTTCallbacks {
    public:
        static void connect(struct mosquitto*, void* thiz, int rc) {
            reinterpret_cast<MQTTAdapter*>(thiz)->onConnect(rc);
        }

        static void disconnect(struct mosquitto*, void* thiz, int rc) {
            reinterpret_cast<MQTTAdapter*>(thiz)->onDisconnect(rc);
        }

        static void subscribe(struct mosquitto*, void* thiz, int mid, int qos, const int* granted_qos) {
            reinterpret_cast<MQTTAdapter*>(thiz)->onSubscribe(mid, qos, granted_qos);
        }

        static void unsubscribe(struct mosquitto*, void* thiz, int mid) {
            reinterpret_cast<MQTTAdapter*>(thiz)->onUnsubscribe(mid);
        }

        static void publish(struct mosquitto*, void* thiz, int mid) {
            reinterpret_cast<MQTTAdapter*>(thiz)->onPublish(mid);
        }

        static void message(struct mosquitto*, void* thiz, const struct mosquitto_message *msg) {
            reinterpret_cast<MQTTAdapter*>(thiz)->onMessage(msg);
        }

        static void logging(struct mosquitto*, void* thiz, int level, const char* str) {
            cerr << "mqtt: [log] " << str << endl; cerr.flush();
        }
    };


    class MQTTFactory : public MQAdapterFactory {
    public:
        MQAdapter* createAdapter(const uri& uri) {
            if (uri.scheme() == "mqtt") {
                auto adapter = new MQTTAdapter();
                adapter->setHost(uri.host());
                auto portStr = uri.port();
                if (!portStr.empty()) {
                    auto port = atoi(portStr.c_str());
                    if (port > 0) {
                        adapter->setPort(port);
                    }
                }
                return adapter;
            }
            return nullptr;
        }

        static MQTTFactory* get() {
            if (_instance == nullptr) {
                lock_guard<mutex> g(_instanceLock);
                if (_instance == nullptr) {
                    _instance = new MQTTFactory();
                }
            }
            return _instance;
        }

    private:
        MQTTFactory() {
            mosquitto_lib_init();
        }

        ~MQTTFactory() {
            mosquitto_lib_cleanup();
        }

        static MQTTFactory* _instance;
        static mutex _instanceLock;
    };

    MQTTFactory* MQTTFactory::_instance;
    mutex MQTTFactory::_instanceLock;

    MQTTAdapter::MQTTAdapter(const string& clientId)
    :   m_clientId(clientId),
        m_host("localhost"), m_port(1883),
        m_keepalive(60),
        m_connOp(nullptr), m_disconnOp(nullptr),
        m_loop(false) {
        m_mosquitto = mosquitto_new(m_clientId.empty() ? nullptr : m_clientId.c_str(), true, this);
        mosquitto_connect_callback_set(m_mosquitto, MQTTCallbacks::connect);
        mosquitto_disconnect_callback_set(m_mosquitto, MQTTCallbacks::disconnect);
        mosquitto_subscribe_callback_set(m_mosquitto, MQTTCallbacks::subscribe);
        mosquitto_unsubscribe_callback_set(m_mosquitto, MQTTCallbacks::unsubscribe);
        mosquitto_publish_callback_set(m_mosquitto, MQTTCallbacks::publish);
        mosquitto_message_callback_set(m_mosquitto, MQTTCallbacks::message);
        //mosquitto_log_callback_set(m_mosquitto, MQTTCallbacks::logging);
    }

    MQTTAdapter::~MQTTAdapter() {
        mosquitto_destroy(m_mosquitto);
    }

    future<void> MQTTAdapter::connect() {
        lock_guard<mutex> g(m_connLock);
        if (m_disconnOp) {
            throw MQTTError("disconnect in-progress");
        }
        if (!m_connOp) {
            m_connOp = promise_or_throw(
                mosquitto_connect_async(m_mosquitto, m_host.c_str(), m_port, m_keepalive),
                "connect error"
            );
            if (!m_loop) {
                mosquitto_loop_start(m_mosquitto);
                m_loop = true;
            }
        }
        return ((promise<void>*)m_connOp)->get_future();
    }

    future<void> MQTTAdapter::disconnect() {
        lock_guard<mutex> g(m_connLock);
        if (!m_disconnOp) {
            m_disconnOp = promise_or_throw(
                mosquitto_disconnect(m_mosquitto),
                "disconnect error"
            );
        }
        return ((promise<void>*)m_disconnOp)->get_future();
    }

    future<void> MQTTAdapter::sub(const string& topic, MsgHandler callback) {
        {
            lock_guard<mutex> g(m_subsLock);
            auto insertion = m_subs.insert(make_pair(topic, list<MsgHandler>()));
            insertion.first->second.push_back(callback);
            if (!insertion.second) {
                promise<void> p;
                p.set_value();
                return p.get_future();
            }
        }
        int mid = 0;
        auto p = promise_or_throw(
            mosquitto_subscribe(m_mosquitto, &mid, topic.c_str(), 0),
            "subscribe error"
        );
        lock_guard<mutex> g(m_subOpsLock);
        m_subOps[mid] = p;
        return p->get_future();
    }

    future<void> MQTTAdapter::unsub(const string& topic) {
        {
            lock_guard<mutex> g(m_subsLock);
            auto it = m_subs.find(topic);
            if (it == m_subs.end()) {
                promise<void> p;
                p.set_value();
                return p.get_future();
            }
            m_subs.erase(it);
        }
        int mid = 0;
        auto p = promise_or_throw(
            mosquitto_unsubscribe(m_mosquitto, &mid, topic.c_str()),
            "unsubscribe error"
        );
        lock_guard<mutex> g(m_subOpsLock);
        m_subOps[mid] = p;
        return p->get_future();
    }

    future<void> MQTTAdapter::pub(const string& topic, const const_buffer& data,
        const PubOptions& opts) {
        int mid = 0;
        auto p = promise_or_throw(
            mosquitto_publish(m_mosquitto, &mid, topic.c_str(),
                buffer_size(data), buffer_cast<const void*>(data),
                0, opts.retain),
            "publish error"
        );
        lock_guard<mutex> g(m_pubOpsLock);
        m_pubOps[mid] = p;
        return p->get_future();
    }

    void MQTTAdapter::onConnect(int rc) {
        lock_guard<mutex> g(m_connLock);
        complete_promise((promise<void>*)m_connOp, rc, "connect error");
        if (rc == MOSQ_ERR_SUCCESS) {
            lock_guard<mutex> g(m_subsLock);
            for (auto& v : m_subs) {
                mosquitto_subscribe(m_mosquitto, nullptr, v.first.c_str(), 0);
            }
        }
        m_connOp = nullptr;
    }

    void MQTTAdapter::onDisconnect(int rc) {
        lock_guard<mutex> g(m_connLock);
        complete_promise((promise<void>*)m_disconnOp, rc, "disconnect error");
        m_disconnOp = nullptr;
        if (rc == 0) {
            mosquitto_loop_stop(m_mosquitto, false);
            m_loop = false;
        }
    }

    void MQTTAdapter::onSubscribe(int mid, int qos, const int* granted_qos) {
        complete_op(m_subOpsLock, m_subOps, mid);
    }

    void MQTTAdapter::onUnsubscribe(int mid) {
        complete_op(m_subOpsLock, m_subOps, mid);
    }

    void MQTTAdapter::onPublish(int mid) {
        complete_op(m_pubOpsLock, m_pubOps, mid);
    }

    void MQTTAdapter::onMessage(const struct mosquitto_message* msg) {
        if (msg->topic == nullptr) {
            return;
        }
        list<MsgHandler> handlers;
        {
            lock_guard<mutex> g(m_subsLock);
            for (auto v : m_subs) {
                bool result = false;
                int rc = mosquitto_topic_matches_sub(v.first.c_str(), msg->topic, &result);
                if (rc == MOSQ_ERR_SUCCESS && result) {
                    for (auto h : v.second) {
                        handlers.push_back(h);
                    }
                }
            }
        }
        if (!handlers.empty()) {
            MQTTMsg m(msg);
            for (auto h : handlers) {
                h(&m);
            }
        }
    }

    MQAdapterFactory* MQTTAdapter::factory() {
        return MQTTFactory::get();
    }
}

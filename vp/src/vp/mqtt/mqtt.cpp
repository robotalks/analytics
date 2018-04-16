#include <cstring>
#include <stdexcept>
#include <opencv2/core/core.hpp>

#include "vp/operators.h"
#include "vp/mqtt/mqtt.h"

namespace vp {
    using namespace std;

    void MQTT::initialize() {
        mosquitto_lib_init();
    }

    void MQTT::cleanup() {
        mosquitto_lib_cleanup();
    }

    class MQTTCallbacks {
    public:
        static void connect(struct mosquitto*, void* thiz, int rc) {
            reinterpret_cast<MQTTClient*>(thiz)->onConnect(rc);
        }

        static void disconnect(struct mosquitto*, void* thiz, int rc) {
            reinterpret_cast<MQTTClient*>(thiz)->onDisconnect(rc);
        }

        static void publish(struct mosquitto*, void* thiz, int mid) {
            reinterpret_cast<MQTTClient*>(thiz)->onPublish(mid);
        }

        /*
        static void subscribe(struct mosquitto*, void* thiz, int mid, int qos, const int* granted_qos) {
            reinterpret_cast<MQTTClient*>(thiz)->onSubscribe(mid, qos, granted_qos);
        }

        static void unsubscribe(struct mosquitto*, void* thiz, int mid) {
            reinterpret_cast<MQTTClient*>(thiz)->onUnsubscribe(mid);
        }

        static void message(struct mosquitto*, void* thiz, const struct mosquitto_message *msg) {
            reinterpret_cast<MQTTClient*>(thiz)->onMessage(msg);
        }

        static void logging(struct mosquitto*, void* thiz, int level, const char* str) {
            //cerr << "mqtt: [log] " << str << endl; cerr.flush();
        }
        */
    };

    static string make_error_msg(int rc, const string& errmsg) {
        char str[128];
        sprintf(str, "mqtt err %d: ", rc);
        return str + errmsg;
    }

    static future<void> make_promise(int rc, const string& errmsg, function<void(promise<void>*)> fn) {
        if (rc != MOSQ_ERR_SUCCESS) {
            promise<void> p;
            p.set_exception(make_exception_ptr(runtime_error(make_error_msg(rc, errmsg))));
            return p.get_future();
        }
        auto p = new promise<void>();
        fn(p);
        return p->get_future();
    }

    static void complete_promise(promise<void>* p, int rc, const string& errmsg) {
        if (p) {
            if (rc == MOSQ_ERR_SUCCESS) {
                p->set_value();
            } else {
                p->set_exception(make_exception_ptr(runtime_error(make_error_msg(rc, errmsg))));
            }
            delete p;
        }
    }

    static void complete_op(mutex& m, unordered_map<int, promise<void>*>& ops, int mid) {
        lock_guard<mutex> g(m);
        auto it = ops.find(mid);
        if (it != ops.end()) {
            it->second->set_value();
            ops.erase(it);
        }
    }

    MQTTClient::MQTTClient(const string& clientId, int keepalive)
    : m_client_id(clientId),
      m_keepalive(keepalive),
      m_conn_op(nullptr),
      m_disconn_op(nullptr),
      m_loop(false) {
        m_mosquitto = mosquitto_new(m_client_id.empty() ? nullptr : m_client_id.c_str(), true, this);
        mosquitto_connect_callback_set(m_mosquitto, MQTTCallbacks::connect);
        mosquitto_disconnect_callback_set(m_mosquitto, MQTTCallbacks::disconnect);
        mosquitto_publish_callback_set(m_mosquitto, MQTTCallbacks::publish);
        //mosquitto_subscribe_callback_set(m_mosquitto, MQTTCallbacks::subscribe);
        //mosquitto_unsubscribe_callback_set(m_mosquitto, MQTTCallbacks::unsubscribe);
        //mosquitto_message_callback_set(m_mosquitto, MQTTCallbacks::message);
        //mosquitto_log_callback_set(m_mosquitto, MQTTCallbacks::logging);
    }

    MQTTClient::~MQTTClient() {
        mosquitto_destroy(m_mosquitto);
    }

    future<void> MQTTClient::connect(const string& host, unsigned short port) {
        lock_guard<mutex> g(m_conn_lock);
        if (m_disconn_op) {
            throw logic_error("disconnect in-progress");
        }
        if (!m_conn_op) {
            return make_promise(
                mosquitto_connect_async(m_mosquitto, host.c_str(), port, m_keepalive),
                "connect error",
                [this] (promise<void>* p) {
                    m_conn_op = p;
                    if (!m_loop) {
                        mosquitto_loop_start(m_mosquitto);
                        m_loop = true;
                    }
                }
            );
        }
        return const_cast<promise<void>*>(m_conn_op)->get_future();
    }

    future<void> MQTTClient::disconnect() {
        lock_guard<mutex> g(m_conn_lock);
        if (!m_disconn_op) {
            return make_promise(
                mosquitto_disconnect(m_mosquitto),
                "disconnect error",
                [this] (promise<void>* p) {
                    m_disconn_op = p;
                }
            );
        }
        return const_cast<promise<void>*>(m_disconn_op)->get_future();
    }

    future<void> MQTTClient::publish(const string& topic, const string& data) {
        int mid = 0;
        return make_promise(
            mosquitto_publish(m_mosquitto, &mid, topic.c_str(),
                data.length(), data.c_str(),
                0, false),
            "publish error",
            [this, &mid] (promise<void>* p) {
                lock_guard<mutex> g(m_pub_ops_lock);
                m_pub_ops[mid] = p;
            }
        );
    }

    void MQTTClient::onConnect(int rc) {
        lock_guard<mutex> g(m_conn_lock);
        complete_promise((promise<void>*)m_conn_op, rc, "connect error");
        m_conn_op = nullptr;
    }

    void MQTTClient::onDisconnect(int rc) {
        lock_guard<mutex> g(m_conn_lock);
        complete_promise((promise<void>*)m_disconn_op, rc, "disconnect error");
        m_disconn_op = nullptr;
        if (rc == 0) {
            mosquitto_loop_stop(m_mosquitto, false);
            m_loop = false;
        }
    }

    void MQTTClient::onPublish(int mid) {
        complete_op(m_pub_ops_lock, m_pub_ops, mid);
    }

    void MQTTClient::Op::operator() (Graph::Ctx ctx) {
        const cv::Mat& m = ctx.in(0)->as<cv::Mat>();
        const ImageId& id = ctx.in(1)->as<ImageId>();
        const vector<DetectBox>& boxes = ctx.in(2)->as<vector<DetectBox>>();
        char cstr[1024];
        sprintf(cstr, "{\"src\":\"%s\",\"seq\":%llu,\"size\":[%d,%d],\"boxes\":[",
            id.src.c_str(), id.seq, m.cols, m.rows);
        string str(cstr);
        for (auto& b : boxes) {
            sprintf(cstr, "{\"class\":%d,\"score\":%f,\"rc\":[%d,%d,%d,%d]},",
                b.category, b.confidence, b.x0, b.y0, b.x1, b.y1);
            str += cstr;
        }
        str = str.substr(0, str.length()-1) + "]}";
        client->publish(topic, str);
    }
}

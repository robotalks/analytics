#ifndef __VP_MQTT_MQTT_H
#define __VP_MQTT_MQTT_H

#include <string>
#include <future>
#include <mutex>
#include <unordered_map>
#include <mosquitto.h>

#include "vp/graph.h"

namespace vp {
    struct MQTT {
        static void initialize();
        static void cleanup();
    };

    class MQTTClient {
    public:
        static constexpr unsigned short PORT = 1883;

        MQTTClient(const ::std::string& clientId = "", int keepalive = 60);
        virtual ~MQTTClient();

        ::std::future<void> connect(const ::std::string& host, unsigned short port = PORT);
        ::std::future<void> disconnect();
        ::std::future<void> publish(const ::std::string& topic, const ::std::string& msg);

        struct Op {
            MQTTClient *client;
            ::std::string topic;
            Op(MQTTClient *c, const ::std::string& t) : client(c), topic(t) { }
            void operator() (Graph::Ctx);
            Graph::OpFunc operator() () const { return *const_cast<Op*>(this); }
        };
    private:
        ::std::string m_client_id;
        int m_keepalive;
        struct mosquitto *m_mosquitto;
        volatile ::std::promise<void>* m_conn_op;
        volatile ::std::promise<void>* m_disconn_op;
        volatile bool m_loop;
        ::std::mutex m_conn_lock;

        ::std::unordered_map<int, ::std::promise<void>*> m_pub_ops;
        ::std::mutex m_pub_ops_lock;

        void onConnect(int);
        void onDisconnect(int);
        void onPublish(int);
        // void onSubscribe(int, int, const int*);
        // void onUnsubscribe(int);
        // void onMessage(const struct mosquitto_message*);

        friend class MQTTCallbacks;
    };
}

#endif

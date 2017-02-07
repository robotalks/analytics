#include <stdexcept>
#include "cmn/pubsub.h"

namespace cmn {
    using namespace std;
    using namespace boost::asio;
    using namespace boost::network::uri;

    MQConnector::MQConnector() {

    }

    MQConnector::~MQConnector() {

    }

    void MQConnector::addFactory(MQAdapterFactory *factory) {
        m_factories.push_back(factory);
    }

    void MQConnector::initialize(Application *app) {
        m_app = app;
        app->options().add_options()
            ("mq-url", "message queue url", cxxopts::value<string>())
        ;
    }

    void MQConnector::start() {
        string url = m_app->opt("mq-url").as<string>();
        if (url.empty()) {
            url = "mqtt://127.0.0.1";
        }
        uri mqUrl(url);
        if (!mqUrl.is_valid()) {
            throw invalid_argument("mq-url: invalid url");
        }
        for (auto factory : m_factories) {
            m_adapter = factory->createAdapter(mqUrl);
            if (m_adapter != nullptr) {
                break;
            }
        }
        if (m_adapter == nullptr) {
            throw invalid_argument("mq-url: unknown adapter");
        }

        m_adapter->connect().wait();
    }

    void MQConnector::stop() {
        m_adapter->disconnect().wait();
    }

    void MQConnector::cleanup() {

    }

    future<void> MQConnector::sub(const string& topic, MsgHandler handler) {
        return m_adapter->sub(topic, handler);
    }

    future<void> MQConnector::unsub(const string& topic) {
        return m_adapter->unsub(topic);
    }

    future<void> MQConnector::pub(const string& topic, const const_buffer& data,
        const PubOptions& opts) {
        return m_adapter->pub(topic, data, opts);
    }

    PubSub::PubSub(MQConnector* connector,
        const string& subTopicOpt, const string &pubTopicOpt)
    : m_conn(connector), m_subOpt(subTopicOpt), m_pubOpt(pubTopicOpt) {

    }

    void PubSub::initialize(Application *app) {
        m_app = app;
        app->options().add_options()
            (m_subOpt, "subscription topic", cxxopts::value<string>())
            (m_pubOpt, "publish topic", cxxopts::value<string>())
        ;
    }

    void PubSub::start() {
        m_sub = m_app->opt(m_subOpt).as<string>();
        m_pub = m_app->opt(m_pubOpt).as<string>();
        if (!m_sub.empty()) {
            m_conn->sub(m_sub, m_handler).wait();
        }
    }

    void PubSub::stop() {
        if (!m_sub.empty()) {
            m_conn->unsub(m_sub).wait();
        }
    }

    void PubSub::cleanup() {

    }

    future<void> PubSub::pub(const ::boost::asio::const_buffer& data, const PubOptions& opts) {
        return pub("", data, opts);
    }

    future<void> PubSub::pub(const ::std::string& subTopic,
        const ::boost::asio::const_buffer& data, const PubOptions& opts) {
        if (m_pub.empty()) {
            return promise<void>().get_future();
        }
        auto topic = m_pub;
        if (!subTopic.empty()) {
            topic += "/" + subTopic;
        }
        return m_conn->pub(topic, data, opts);
    }
}

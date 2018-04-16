#include <stdexcept>

#include "vp/graph.h"

namespace vp {
    using namespace std;

    Graph::Var::Var(const string& name)
    : m_name(name), m_val(nullptr) {
    }

    Graph::Var::~Var() {
        if (m_val) {
            delete m_val;
        }
    }

    void Graph::Var::set_val(Val *val) {
        clear();
        m_val = val;
    }

    void Graph::Var::clear() {
        auto p = m_val;
        m_val = nullptr;
        if (p != nullptr) delete p;
    }

    Graph::Var* Graph::Var::must_set() {
        if (!is_set()) throw logic_error("variable not set: " + m_name);
        return this;
    }

    const Graph::Var* Graph::Var::must_set() const {
        return const_cast<Var*>(this)->must_set();
    }
}

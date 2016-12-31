#pragma once

#include <iostream>
#include <chrono>

namespace cmn {
    class PerfLogger {
    public:
        PerfLogger(const char *name)
        : m_name(name) {
            m_start = ::std::chrono::high_resolution_clock::now();
        }

        ~PerfLogger() {
            auto end = ::std::chrono::high_resolution_clock::now();
            ::std::cerr << m_name << ": "
                << ::std::chrono::duration_cast<::std::chrono::microseconds>(end - m_start).count()
                << ::std::endl;
            ::std::cerr.flush();
        }

    private:
        const char *m_name;
        ::std::chrono::time_point<::std::chrono::high_resolution_clock> m_start;
    };
}

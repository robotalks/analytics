#include <unistd.h>
#include <limits.h>
#include <exception>
#include <iostream>
#include <functional>
#include "cmn/app.h"

namespace cmn {

static const char* appName(const char *argv0) {
    const char *p = strrchr(argv0, '/');
    return p == NULL ? argv0 : p;
}

static ::std::string exeFullPath(const char *argv0) {
    char exePath[PATH_MAX];
    if (readlink("/proc/self/exe", exePath, sizeof(exePath)) >= 0) {
        exePath[sizeof(exePath) - 1] = 0;
        return exePath;
    } else {
        return argv0;
    }
}

class ModuleLifecycle {
public:
    ModuleLifecycle(::std::vector<AppModule*> modules)
    : m_modules(modules), m_startedTo(-1) {

    }

    ~ModuleLifecycle() {
        reverse(m_startedTo, [](AppModule* module) { module->stop(); });
        reverse((int)m_modules.size() - 1, [](AppModule* module) { module->cleanup(); });
    }

    void start() {
        for (auto module : m_modules) {
            module->start();
            m_startedTo ++;
        }
    }

private:
    ::std::vector<AppModule*> m_modules;
    size_t m_startedTo;

    void reverse(int start, std::function<void(AppModule*)> fn) {
        for (int i = start; i >= 0; i --) {
            try {
                fn(m_modules[i]);
            } catch (std::exception& e) {
                std::cerr << e.what() << std::endl;
            }
        }
    }
};

Application::Application(int argc, char **argv)
: m_argc(argc), m_argv(argv),
  m_name(appName(argv[0])), m_exeFile(exeFullPath(argv[0])),
  m_options(m_name) {
      m_options.add_options()("help", "help", cxxopts::value<bool>());
}

int Application::main() {
    m_options.parse(m_argc, m_argv);
    if (opt("help").as<bool>()) {
        std::cout << m_options.help() << std::endl;
        return 2;
    }
    ModuleLifecycle modules(m_modules);
    int exitCode = 0;
    try {
        modules.start();
        exitCode = run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        throw;
    }
    return exitCode;
}

::std::string Application::exeDir() const {
    auto pos = m_exeFile.find_last_of("/");
    if (pos != ::std::string::npos) {
        return m_exeFile.substr(0, pos);
    }
    return "";
}

void Application::addModule(AppModule *module) {
    module->initialize(this);
    m_modules.push_back(module);
}

}

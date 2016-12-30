#pragma once

#include <string>
#include <vector>
#include "cxxopts.hpp"
#include "common.h"

namespace cmn {

class Application;

struct AppModule : Interface {
    virtual void initialize(Application*) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void cleanup() = 0;
};

class Application {
public:
    Application(int argc, char **argv);
    virtual int main();

    ::cxxopts::Options& options() { return m_options; }
    const ::cxxopts::OptionDetails opt(const ::std::string& name) const { return m_options[name]; }

    const ::std::string& name() const { return m_name; }
    const ::std::string& exeFile() const { return m_exeFile; }
    ::std::string exeDir() const;

    void addModule(AppModule*);

protected:
    int m_argc;
    char** m_argv;

    virtual int run() { return 0; };

private:
    ::std::string m_name;
    ::std::string m_exeFile;
    ::cxxopts::Options m_options;
    ::std::vector<AppModule*> m_modules;
};

}

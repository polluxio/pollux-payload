#pragma once

#include <iostream>
#include <string>
#include "pollux.h"
#include <fstream>

#define LAMBDA_BODY(x) #x

#define RUN_POLLUX(lambda) \
    auto toRun = lambda; \
    std::string toRunStr = LAMBDA_BODY(lambda);\
    std::string fileName = std::string("config.yaml");\
    ofstream file(fileName.c_str());\
    file << "synchronized: true\n";\
    file << "executor: local\n";\
    file << "payload:  /home/nonoc/nativeC/payload/pollux-payload/build/src/c++/examples/test/pollux-payload-test\n";\
    file << "port: 50000\n";\
    file << "user_options:\n";\
    file << " code:\n";\
    file << "  type: string\n";\
    file << "  value: \"#include <iostream>\\n using namespace std;\\n int main() { auto code =" << toRunStr   << "; code(); return 0;}\"\n";\
    file << "payloads_nb: 3\n";\
    file.close();\
    char *config = (char*)("./config.yaml");\
    runPollux(config);



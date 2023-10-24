// SPDX-FileCopyrightText: 2023 Pollux authors <https://github.com/polluxio/pollux-payload/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include <sstream>
#include <thread>
#include "spdlog/spdlog.h"

#include "pollux.h"
#include <iostream>
#include <fstream>
using namespace std;

namespace {

class PolluxPayloadTest: public PolluxPayload {
  public:
    PolluxPayloadTest(): PolluxPayload("pollux-payload-test") {}

    void init(ZebulonPayloadClient* client) override {
      auto maxIterationsOption = getUserOptionValue("nb_iterations");
      if (maxIterationsOption) {
        if (not (maxIterationsOption->index() == UserOptionType::LONG)) {
          throw PolluxPayloadException("wrong nb_iterations option type");
        }
        maxIterations_ = std::get<UserOptionType::LONG>(*maxIterationsOption);
      }
      spdlog::info("Number of iterations: {}", maxIterations_);
    }

    void loop(ZebulonPayloadClient* client) override {
          // Create a C++ file named hello.cpp
          std::string code;
          auto functionOption = getUserOptionValue("code");
          if (functionOption) {
            if (not (functionOption->index() == UserOptionType::STRING)) {
              throw PolluxPayloadException("wrong function option type: should be string");
            }
            code = std::get<UserOptionType::STRING>(*functionOption);
          }
          std::string fileName = std::string("code") + std::to_string(getLocalID()) +  std::string(".cpp");
          ofstream file(fileName.c_str());
          file << code;
          file.close();
          std::string command = std::string("g++ -o code") + std::to_string(getLocalID()) +  std::string(" ")+  fileName;
          system(command.c_str());
          // Run the executable using the system command
          std::string runCommand = std::string("./code") + std::to_string(getLocalID());
          system(runCommand.c_str());
          client->sendPayloadLoopEnd(iteration_++);
    }

    void transmit(const pollux::PolluxMessage* message) override {
      std::ostringstream logMessage;
      logMessage << "Pollux Message received: "
        << "origin=" << message->origin()
        << " key=" << message->key();
      switch (message->value_case()) {
        case pollux::PolluxMessage::kStrValue:
          logMessage << ", value=" << message->strvalue() << std::endl;
          break;
        case pollux::PolluxMessage::kInt64Value:
          logMessage << ", value=" << message->int64value() << std::endl;
          break;
        case pollux::PolluxMessage::kInt64ArrayValue:
          logMessage << ", value=";
          for (auto v: message->int64arrayvalue().values()) {
            logMessage << v << ", ";
          }
          logMessage << std::endl;
          break;
        case pollux::PolluxMessage::kDoubleArrayValue:
          logMessage << ", value=";
          for (auto v: message->doublearrayvalue().values()) {
            logMessage << v << ", ";
          }
          logMessage << std::endl;
          break;
        default:
          break;
      }
      spdlog::info("{}", logMessage.str());
    }
    
  private:
    int                     iteration_      {1};
    int                     maxIterations_  {5};
};

}

int main(int argc, char** argv) {
  auto payload = std::make_unique<PolluxPayloadTest>();
  return Pollux::Main(argc, argv, payload.get());
}

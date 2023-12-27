// SPDX-FileCopyrightText: 2023 Pollux authors <https://github.com/polluxio/pollux-payload/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include <sstream>
#include <thread>
#include "spdlog/spdlog.h"

#include "pollux.h"

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
      spdlog::info("Main loop started iteration: {} synchronized: {}", iteration_, isSynchronized());
      int nbMessages = 0;
      while (1) { //4x communication
        using namespace std::chrono_literals;
        //use more random sleep
        std::this_thread::sleep_for(5000ms);
        int pick = rand() % getOtherIDs().size();
        int id = getOtherIDs()[pick];
        ZebulonPayloadClient::NodeStatus status = client->getNodeStatus(id);

        spdlog::info("Logging {}", nbMessages);

        int messageType = rand() % 4;
        switch(messageType) {
          case 0:
            {
              //random int
              int value = rand()%std::numeric_limits<int>::max();
              client->transmit(id, "int64", (int64_t)value);
              client->polluxLog(std::to_string(id), std::to_string(value));
              break;
            }
          case 1:
            {
              //random uint
              int value = rand()%std::numeric_limits<int>::max();
              client->transmit(id, "uint64", (uint64_t)value);
              client->polluxLog(std::to_string(id), std::to_string(value));
              break;
            }
          case 2:
            {
              //int array
              client->transmit(id, "int64array", ZebulonPayloadClient::Int64Array({0, 10, 200, 3000, 40000, 500000}));
              client->polluxLog(std::to_string(id), "int64array");
              break;
            }
          default:
            client->transmit(id, "string", "value");
            client->polluxLog(std::to_string(id), "value");
            break;

        }

        spdlog::info("Message {}", nbMessages++);
        if (isSynchronized() and nbMessages > 4) {
          break;
        }
      }
      if (isSynchronized()) {
        if (iteration_ > maxIterations_) {
          client->sendPayloadLoopEnd(iteration_++);
        } else {
          client->sendPayloadLoopReadyForNextIteration(iteration_++);
        }
      }
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

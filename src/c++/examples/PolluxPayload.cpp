#include <sstream>
#include <thread>
#include "spdlog/spdlog.h"

#include "pollux.h"

namespace {

class PolluxPayloadExample: public PolluxPayload {
  public:
    PolluxPayloadExample(): PolluxPayload("pollux-payload-example") {}

    void init() override {
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

        spdlog::info("Reporting {}", nbMessages);
        client->polluxReport("key", "value");

        int messageType = rand() % 4;
        switch(messageType) {
          case 0:
            {
              //random int
              int value = rand()%std::numeric_limits<int>::max();
              client->polluxCommunication(id, "int64", (int64_t)value);
              break;
            }
          case 1:
            {
              //random uint
              int value = rand()%std::numeric_limits<int>::max();
              client->polluxCommunication(id, "uint64", (uint64_t)value);
              break;
            }
          case 2:
            {
              //int array
              client->polluxCommunication(id, "int64array", ZebulonPayloadClient::Int64Array({0, 10, 200, 3000, 40000, 500000}));
              break;
            }
          default:
            client->polluxCommunication(id, "string", "value");
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

    void polluxCommunication(const pollux::PolluxMessage* message) override {
      std::ostringstream logMessage;
      logMessage << "Pollux Message received: "
        << "origin=" << message->origin()
        << " key=" << message->key();
      switch (message->value_case()) {
        case pollux::PolluxMessage::kStrValue:
          logMessage << ", value=" << message->strvalue() << std::endl;
          break;
        case pollux::PolluxMessage::kUint64Value:
          logMessage << ", value=" << message->uint64value() << std::endl;
          break;
        case pollux::PolluxMessage::kInt64ArrayValue:
          logMessage << ", value=";
          for (auto v: message->int64arrayvalue().values()) {
            logMessage << v << ", ";
          }
          logMessage << std::endl;
          break;
        case pollux::PolluxMessage::kInt64Value:
          logMessage << ", value=" << message->int64value() << std::endl;
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
  auto payload = std::make_unique<PolluxPayloadExample>();
  return Pollux::Main(argc, argv, payload.get());
}

#include <sstream>
#include <thread>
#include "spdlog/spdlog.h"

#include "pollux.h"

namespace {

class PolluxPayloadExample: public PolluxPayload {
  public:
    PolluxPayloadExample(): PolluxPayload("polllux-payload-example") {}

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
        client->polluxReport(id, "key", "value");
        spdlog::info("Message {}", nbMessages++);
        client->polluxCommunication(id, "key", "value");
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

    void polluxCommunication(const pollux::PolluxMapMessage* message) override {
      std::ostringstream logMessage;
      logMessage << "Pollux Message received: "
        << "origin=" << message->origin();
      for (auto& [key, value]: message->map()) {
        logMessage << ", key=" << key;
        switch (value.value_case()) {
          case pollux::PolluxMessageValue::kStrValue:
            logMessage << ", value=" << value.strvalue() << std::endl;
            break;
          default:
            break;
        }
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

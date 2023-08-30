#include <thread>
#include "spdlog/spdlog.h"

#include "pollux.h"

namespace {

class PolluxPayloadExample: public PolluxPayload {
  public:
    PolluxPayloadExample() = default;

    void loop(ZebulonPayloadClient* client) override {
      spdlog::info("Main loop started iteration: {} synchronized: {}", iteration_, isSynchronized());
      int nbMessages = 0;
      while (1) { //4x communication
        using namespace std::chrono_literals;
        //use more random sleep
        std::this_thread::sleep_for(5000ms);
        int pick = rand() % getOtherIDs().size();
        int id = getOtherIDs()[pick];
        if (reporting_) {
          spdlog::info("Reporting {}", nbMessages);
          client->polluxReport(id, "key", "value");
        }
        spdlog::info("Message {}", nbMessages++);
        client->polluxCommunication(id, "key", "value");
        if (isSynchronized() and nbMessages > 4) {
          break;
        }
      }
      if (isSynchronized()) {
        if (iteration_ > 5) {
          client->sendPayloadLoopEnd(iteration_++);
        } else {
          client->sendPayloadLoopReadyForNextIteration(iteration_++);
        }
      }
    }

  private:
    bool                    reporting_ = true;
    int                     iteration_ = 0;
};

}

int main(int argc, char** argv) {
  auto payload = std::make_unique<PolluxPayloadExample>();
  return Pollux::Main(argc, argv, payload.get());
}

#include <getopt.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <numeric>

#include <plog/Log.h>
#include <plog/Initializers/RollingFileInitializer.h>

#include <grpcpp/grpcpp.h>

#include "pollux_payload.grpc.pb.h"

#include "ZebulonPayloadClient.h"
#include "PolluxAppException.h"

namespace {

std::vector<int> otherIDs;

void printHelp() {
  std::cout
    << "--port <p>          Pollux master port" << std::endl
    << "--id <id>           Pollux payload id" << std::endl
    << "--partitions <nb>   Global number of partitions" << std::endl 
    << "--help              Displays this help and exit" << std::endl
    << "--synchronized      Synchronous mode" << std::endl
    << "--alone:            Do not attempt to connect to Pollux master port (only useful for debugging)" << std::endl;
  exit(1);
}

class UserPayLoad {
  public:
    UserPayLoad() = default;
    UserPayLoad(const UserPayLoad&) = default;

    void setSynchronized(bool mode) {
      synchronized_ = mode;
    }

    void loop(ZebulonPayloadClient* client) {
      PLOG_INFO << "Main loop started iteration:" << iteration_ << " synchronized:" << synchronized_;
      int nbMessages = 0;
      while (1) { //4x communication
        using namespace std::chrono_literals;
        //use more random sleep
        std::this_thread::sleep_for(5000ms);
        int pick = rand() % otherIDs.size();
        int id = otherIDs[pick];
        PLOG_INFO << "Message " << nbMessages++;
        client->polluxCommunication(id, "key", "value");
        if (synchronized_ and nbMessages > 4) {
          break;
        }
      }
      if (synchronized_) {
        if (iteration_ > 5) {
          client->sendPayloadLoopEnd(iteration_++);
        } else {
          client->sendPayloadLoopReadyForNextIteration(iteration_++);
        }
      }
    }
  private:
    bool  synchronized_ = false;
    int   iteration_ = 0;
};


class PolluxPayloadService final : public pollux::PolluxPayload::Service {
  public:
    PolluxPayloadService() = delete;
    PolluxPayloadService(const PolluxPayloadService&) = delete;
    PolluxPayloadService(ZebulonPayloadClient* client, const UserPayLoad& payload):
      userPayLoad_(payload),
      zebulonClient_(client) {}
    grpc::Status Terminate(
      grpc::ServerContext* context,
      const pollux::PayloadTerminateMessage* messsage, 
      pollux::EmptyResponse* response) override {
      //Terminate local app
      //should the server be gracefully shutdown ??  
      return grpc::Status::OK;
    }

    grpc::Status Start(
      grpc::ServerContext* context,
      const pollux::PayloadStartMessage* messsage, 
      pollux::PolluxStandardResponse* response) override {
      PLOG_INFO << "Start payload received";
      std::thread mainLoopThread(&UserPayLoad::loop, &userPayLoad_, zebulonClient_);
      mainLoopThread.detach();
      response->set_info("Payload has been started");
      return grpc::Status::OK;
    }

    grpc::Status PolluxCommunication(
      grpc::ServerContext* context,
      const pollux::PolluxMapMessage* message,
      pollux::PolluxMessageResponse* response) override {
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
      PLOG_INFO << logMessage.str();
      response->set_info(logMessage.str() + " understood");
      return grpc::Status::OK;
    }
  private:
    ZebulonPayloadClient* zebulonClient_  {nullptr};
    UserPayLoad           userPayLoad_    {};
};

}

int main(int argc, char **argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  const char* const short_opts = "pin:hsa::";
  const option long_opts[] = {
    {"port",          required_argument,  0,        'p'},
    {"id",            required_argument,  0,        'i'},
    {"partitions",    required_argument,  0,        'n'},
    {"synchronized",  no_argument,        nullptr,  's'},
    {"alone",         no_argument,        nullptr,  'a'},
    {"help" ,         no_argument,        nullptr,  'h'},
    {nullptr,         no_argument,        nullptr,  0}
  };

  int zebulonPort = -1;
  int id = -1;
  int partitions = -1;
  bool synchronized = false;
  bool alone = false;

  while (true) {
    const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
    if (opt == -1) {
      break;
    }

    switch (opt) {
      case 'p':
        zebulonPort = std::stoi(optarg);
        break;
      case 'i':
        id = std::stoi(optarg);
        break;
      case 'n':
        partitions = std::stoi(optarg);
        break;
      case 'a':
        alone = true;
        break;
      case 's':
        synchronized = true;
        break;
      case 'h':
        printHelp();
        break;
      default:
        std::cout << opt << " is not a recognized option, recognized options are:" << std::endl;
        printHelp();
        break;
    }
  }

  if (zebulonPort == -1) {
    std::cout << "port argument is mandatory" << std::endl;
    printHelp();
  }
  if (id == -1) {
    std::cout << "id argument is mandatory" << std::endl;
    printHelp();
  }
  if (partitions == -1) {
    std::cout << "partitions argument is mandatory" << std::endl;
    printHelp();
  }

  std::string logFileName("pollux-app-" + std::to_string(id) + ".log");
  plog::init(plog::debug, logFileName.c_str(), 0);
  PLOG_INFO << "########################################################";
  PLOG_INFO << logFileName;
  PLOG_INFO << "########################################################";
  {
    std::ostringstream stream;
    for (int i=0; i<argc; i++) {
      stream << argv[i] << " ";
    }
    PLOG_INFO << "Command line: " << stream.str();
  }

  if (synchronized) {
    PLOG_INFO << "Synchronized mode is on";
  }

  try {
    int localID = id;
    otherIDs = std::vector<int>(partitions);
    std::iota(otherIDs.begin(), otherIDs.end(), 0);
    otherIDs.erase(std::remove(otherIDs.begin(), otherIDs.end(), localID), otherIDs.end());

    //ServerAddress serverAddress = getAvailableAddress();
    ZebulonPayloadClient* zebulonClient = nullptr;

    std::string zebulonAddress("localhost");
    zebulonAddress += ":" + std::to_string(zebulonPort);

    if (not alone) {
      PLOG_INFO << "creating local client";
      zebulonClient = new ZebulonPayloadClient(
        grpc::CreateChannel(zebulonAddress, grpc::InsecureChannelCredentials()),
        localID
      );
    }

    std::string localServerAddress("localhost:0");
    //Logger::info("starting server on " + serverAddress.getString());
    UserPayLoad userPayLoad;
    userPayLoad.setSynchronized(synchronized);
    PolluxPayloadService service(zebulonClient, userPayLoad);
    grpc::ServerBuilder builder;
    //AddListeningPort last optional arg: If not nullptr, gets populated with the port number bound to the grpc::Server
    //for the corresponding endpoint after it is successfully bound by BuildAndStart(), 0 otherwise.
    //AddListeningPort does not modify this pointer.
    int serverPort = 0;
    builder.AddListeningPort(localServerAddress, grpc::InsecureServerCredentials(), &serverPort);
    //Explore the following later
    //builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 2000);
    //builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 1000);
    //builder.AddChannelArgument(GRPC_ARG_HTTP2_BDP_PROBE, 1);
    //builder.AddChannelArgument(GRPC_ARG_MAX_CONNECTION_IDLE_MS , 1000);
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    PLOG_INFO << "starting server on " << std::to_string(serverPort);
    if (not server.get()) {
      std::ostringstream message;
      message << "GRPC Server could not be started on " << std::to_string(serverPort);
      throw PolluxAppException(message.str());
    }

    if (not alone) {
      PLOG_INFO << "contacting zebulon on " << zebulonAddress << " and sending ready message";
      //I'm alive send message
      zebulonClient->sendPayloadReady(serverPort);
    }

    //create and run local server
    PLOG_INFO << "Server listening on " << std::to_string(serverPort);
    server->Wait(); //blocking

    //we get there if PolluxPayloadService was terminated
    PLOG_INFO << "Server " << std::to_string(serverPort) << " terminated";

    delete zebulonClient;

    // Optional:  Delete all global objects allocated by libprotobuf.
    google::protobuf::ShutdownProtobufLibrary();
  } catch (PolluxAppException& e) {
    PLOG_ERROR << "PolluxAppException: " << e.getReason();
  } catch (std::exception& e) {
    PLOG_ERROR << e.what();
  }
  PLOG_INFO << "End of " << logFileName;
  return 0;
}

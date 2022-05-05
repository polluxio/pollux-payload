#include <getopt.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <numeric>
#include <filesystem>

#include <grpcpp/grpcpp.h>

#include "pollux_payload.grpc.pb.h"

#include "PolluxAppException.h"

namespace {

std::ofstream logFile;
std::vector<int> otherIDs;

class Logger {
  public:
    static void info(const std::string& message) {
      logFile << message << std::endl;
    }
    static void error(const std::string& message) {
      logFile << message << std::endl;
    }
};

void printHelp() {
  std::cout
    << "--port <p>:         Pollux master port" << std::endl
    << "--id <id>:          Pollux payload id" << std::endl
    << "--partitions <nb>:  Global number of partitions" << std::endl 
    << "--help:             Displays this help and exit" << std::endl
    << "--alone:            Do not attempt to connect to Pollux master port (only useful for debugging)" << std::endl;
  exit(1);
}

class ZebulonPayloadClient {
  public:
    ZebulonPayloadClient(
      std::shared_ptr<grpc::Channel> channel,
      int id):
        stub_(pollux::ZebulonPayload::NewStub(channel)),
        id_(id)
    {}

    void sendPayloadReady(uint16_t port) {
      grpc::ClientContext context;
      pollux::PolluxVersion version;
      version.set_version(pollux::PolluxVersion_Version::PolluxVersion_Version_CURRENT);
      pollux::PayloadReadyMessage request;
      request.set_info("I'm alive from: " + std::to_string(id_));
      request.set_port(port);
      request.set_allocated_version(&version);
      pollux::PayloadReadyResponse response;
      grpc::Status status = stub_->PayloadReady(&context, request, &response);
      if (not status.ok()) {
        Logger::error("Error while sending \"sendPayloadReady\": " + status.error_message());
        exit(-54);
      }
      Logger::info("Response from Zebulon to PayloadReady:: " + response.info());
    }

    void polluxCommunication(int id, const std::string& key, const std::string& value) {
      grpc::ClientContext context;
      pollux::PolluxMessage message;
      message.set_origin(id_);
      message.add_destinations(id);
      message.set_key(key);
      message.set_value(value);
      pollux::PolluxMessageResponse response;
      grpc::Status status = stub_->PolluxCommunication(&context, message, &response);
      Logger::info("PolluxCommunication::Response: " + response.info());
    }

    std::string getString() const {
      return "ZebulonPayloadClient id: " + std::to_string(id_);
    }

  private:
    std::unique_ptr<pollux::ZebulonPayload::Stub> stub_;
    int                                             id_;
};

void sendPolluxCommunication(
  ZebulonPayloadClient* client,
  int destination,
  const std::string& key,
  const std::string& value) {
  client->polluxCommunication(destination, key, value);
}

void mainLoop(ZebulonPayloadClient* client) {
  Logger::info("Main loop started");
  while(1) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(5000ms);
    int pick = rand() % otherIDs.size();
    int id = otherIDs[pick];
    sendPolluxCommunication(client, id, "key", "value");
  }
}

class PolluxPayloadService final : public pollux::PolluxPayload::Service {
  public:
    PolluxPayloadService() = delete;
    PolluxPayloadService(const PolluxPayloadService&) = delete;
    PolluxPayloadService(ZebulonPayloadClient* client): zebulonClient_(client) {}
    grpc::Status Terminate(
      grpc::ServerContext* context,
      const pollux::PayloadTerminateMessage* messsage, 
      pollux::PayloadTerminateResponse* response) override {
      //Terminate local app
      //should the server be gracefully shutdown ??  
      return grpc::Status::OK;
    }

    grpc::Status Start(
      grpc::ServerContext* context,
      const pollux::PayloadStartMessage* messsage, 
      pollux::PayloadStartResponse* response) override {
      Logger::info("Start payload received");
      std::thread mainLoopThread(mainLoop, zebulonClient_);
      mainLoopThread.detach();
      response->set_info("Payload has been started");
      return grpc::Status::OK;
    }

    grpc::Status PolluxCommunication(
      grpc::ServerContext* context,
      const pollux::PolluxMessage* message,
      pollux::PolluxMessageResponse* response) override {
      std::ostringstream logMessage;
      logMessage << "Pollux Message received: "
        << "origin=" << message->origin()
        << ", key=" << message->key()
        << ", value=" << message->value();
      Logger::info(logMessage.str());
      response->set_info(logMessage.str() + " understood");
      return grpc::Status::OK;
    }
  private:
    ZebulonPayloadClient* zebulonClient_ {nullptr};
};

}

int main(int argc, char **argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  const char* const short_opts = "pin:ha::";
  const option long_opts[] = {
    {"port",        required_argument,  0,        'p'},
    {"id",          required_argument,  0,        'i'},
    {"partitions",  required_argument,  0,        'n'},
    {"alone",       no_argument,        nullptr,  'a'},
    {"help" ,       no_argument,        nullptr,  'h'},
    {nullptr,       no_argument,        nullptr,  0}
  };

  int zebulonPort = -1;
  int id = -1;
  int partitions = -1;
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
      case 'h':
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
  std::filesystem::path logFilePath(logFileName);
  logFile.open(logFilePath);
  Logger::info("########################################################");
  Logger::info(logFileName);
  Logger::info("########################################################");

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
      Logger::info("creating local client");
      zebulonClient = new ZebulonPayloadClient(
        grpc::CreateChannel(zebulonAddress, grpc::InsecureChannelCredentials()),
        localID
      );
    }

    std::string localServerAddress("localhost:0");
    //Logger::info("starting server on " + serverAddress.getString());
    PolluxPayloadService service(zebulonClient);
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
    Logger::info("starting server on " + std::to_string(serverPort));
    if (not server.get()) {
      std::ostringstream message;
      message << "GRPC Server could not be started on " << std::to_string(serverPort);
      throw PolluxAppException(message.str());
    }

    if (not alone) {
      Logger::info("contacting zebulon on " + zebulonAddress + " and sending ready message");
      //I'm alive send message
      zebulonClient->sendPayloadReady(serverPort);
    }

    //create and run local server
    { 
      std::ostringstream message;
      message << "Server listening on " << std::to_string(serverPort);
      Logger::info(message.str());
    }
    server->Wait(); //blocking

    //we get there if PolluxPayloadService was terminated
    {
      std::ostringstream message;
      message << "Server " << std::to_string(serverPort) << " terminated";
      Logger::info(message.str());
    }

    delete zebulonClient;

    // Optional:  Delete all global objects allocated by libprotobuf.
    google::protobuf::ShutdownProtobufLibrary();
  } catch (PolluxAppException& e) {
    Logger::error("PolluxAppException: " + e.getReason());
  } catch (std::exception& e) {
    Logger::error(e.what());
  }

  Logger::info("End of " + logFileName);
  return 0;
}

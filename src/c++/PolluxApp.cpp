#include <getopt.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <numeric>
#include <sys/socket.h>
#include <netinet/in.h>

#include <grpcpp/create_channel.h>
#include <grpcpp/server_builder.h>

#include "pollux_payload.grpc.pb.h"

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
    << "--help:             Displays this help and exit" << std::endl;
  exit(1);
}

class ZebulonPayloadClient {
  public:
    ZebulonPayloadClient(
      std::shared_ptr<grpc::Channel> channel,
      int id,
      int port):
        stub_(pollux::ZebulonPayload::NewStub(channel)),
        id_(id),
        port_(port)
    {}

    void sendPayloadReady(int port) {
      grpc::ClientContext context;
      //pollux::PolluxVersion version;
      //version.set_version(pollux::PolluxVersion_Version::PolluxVersion_Version_CURRENT);
      pollux::PayloadReadyMessage request;
      request.set_info("I'm alive from: " + std::to_string(id_));
      request.set_port(port);
      //request.set_allocated_version(&version);
      pollux::PayloadReadyResponse response;
      grpc::Status status = stub_->PayloadReady(&context, request, &response);
      if (not status.ok()) {
        Logger::error("Error while sending \"sendPayloadReady\": " + status.error_message());
        exit(-54);
      }
      Logger::info("PayloadReady::Response: " + response.info());
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
      return "ZebulonPayloadClient id: " + std::to_string(id_) + " port: " + std::to_string(port_);
    }

  private:
    std::unique_ptr<pollux::ZebulonPayload::Stub> stub_;
    int                                             id_;
    int                                             port_;
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
    sleep(5);
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

int getAvailablePort() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    return -1;
  }

  int port = 0; //get next available port
  struct sockaddr_in sin;

  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = 0;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_family = AF_INET;

  if (bind(sock, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) == -1) {
    if (errno == EADDRINUSE) {
      std::cerr << "Port in use" << std::endl;
      return -1;
    }
    std::cerr << "Current port: " << ntohs(sin.sin_port) << std::endl;
  }

  {
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(sock, (struct sockaddr *)&sin, &len) != -1) {
      Logger::info("port number " + std::to_string((sin.sin_port)));
      return ntohs(sin.sin_port);
    }
  }
  return -1;
}

}

int main(int argc, char **argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  const char* const short_opts = "pin:h::";
  const option long_opts[] = {
    {"port",        required_argument,  0,        'p'},
    {"id",          required_argument,  0,        'i'},
    {"partitions",  required_argument,  0,        'n'},
    {"help" ,       no_argument,        nullptr,  'h'},
    {nullptr,       no_argument,        nullptr,  0}
  };

  int masterPort = -1;
  int id = -1;
  int partitions = -1;

  while (true) {
    const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
    if (opt == -1) {
      break;
    }

    switch (opt) {
      case 'p':
        masterPort = std::stoi(optarg);
        break;
      case 'i':
        id = std::stoi(optarg);
        break;
      case 'n':
        partitions = std::stoi(optarg);
        break;
      case 'h':
      default:
        std::cout << opt << " is not a recognized option, recognized options are:" << std::endl;
        printHelp();
        break;
    }
  }

  if (masterPort == -1) {
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

  int localID = id;
  otherIDs = std::vector<int>(partitions);
  std::iota(otherIDs.begin(), otherIDs.end(), 0);
  otherIDs.erase(std::remove(otherIDs.begin(), otherIDs.end(), localID), otherIDs.end());

  std::string logFileName("pollux-app-" + std::to_string(id) + ".log");
  std::filesystem::path logFilePath(logFileName);
  logFile.open(logFilePath);
  Logger::info("########################################################");
  Logger::info(logFileName);
  Logger::info("########################################################");

  int localPort = getAvailablePort();
  std::string address("localhost");
  address += ":" + std::to_string(masterPort);
  Logger::info("creating local client");
  ZebulonPayloadClient client(
    grpc::CreateChannel(address, grpc::InsecureChannelCredentials()),
    localID,
    localPort
  );

  Logger::info("contacting zebulon on " + address + " and sending ready message");
  //I'm alive send message
  client.sendPayloadReady(localPort);

  //create and run local server
  Logger::info("starting server on " + std::to_string(localPort));
  std::string serverAddress("localhost:");
  serverAddress += std::to_string(localPort);
  PolluxPayloadService service(&client);
  grpc::ServerBuilder builder;
  builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
  //Explore the following later
  //builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 2000);
  //builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 1000);
  //builder.AddChannelArgument(GRPC_ARG_HTTP2_BDP_PROBE, 1);
  //builder.AddChannelArgument(GRPC_ARG_MAX_CONNECTION_IDLE_MS , 1000);
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  { 
    std::ostringstream message;
    message << "Server listening on " << serverAddress;
    Logger::info(message.str());
  }
  server->Wait(); //blocking

  //we get there if PolluxPayloadService was terminated
  {
    std::ostringstream message;
    message << "Server " << serverAddress << " terminated";
    Logger::info(message.str());
  }

  // Optional:  Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();
  Logger::info("End of " + logFileName);
  return 0;
}

#include <getopt.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>

#include <grpcpp/create_channel.h>
#include <grpcpp/server_builder.h>

#include "escher_payload.grpc.pb.h"

namespace {

std::ofstream logFile;

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
    << "--port <p>: Escher master port" << std::endl
    << "--id <id>:  Escher payload id" << std::endl
    << "--help:     Displays this help" << std::endl;
  exit(1);
}

class EscherPayloadService final : public escher::EscherPayload::Service {
  public:
    grpc::Status Terminate(
      grpc::ServerContext* context,
      const escher::PayloadTerminateMessage* messsage, 
      escher::PayloadTerminateResponse* response) override {
      //Terminate local app
      //should the server be gracefully shutdown ??  
      return grpc::Status::OK;
    }

    grpc::Status Start(
      grpc::ServerContext* context,
      const escher::PayloadStartMessage* messsage, 
      escher::PayloadStartResponse* response) override {
      Logger::info("Start payload received");
      return grpc::Status::OK;
    }

    grpc::Status EscherCommunication(
      grpc::ServerContext* context,
      const escher::EscherMessage* message,
      escher::EscherMessageResponse* response) override {
        std::ostringstream logMessage;
        logMessage << "Escher Message received: "
          << "origin=" << message->origin()
          << ", key=" << message->key()
          << ", value=" << message->value();
        Logger::info(logMessage.str());
        return grpc::Status::OK;
      }
};

class EscherletPayloadClient {
  public:
    EscherletPayloadClient(
      std::shared_ptr<grpc::Channel> channel,
      int id,
      int port):
        stub_(escher::EscherletPayload::NewStub(channel)),
        id_(id),
        port_(port)
    {}

    void sendPayloadReady(int port) {
      grpc::ClientContext context;
      escher::PayloadReadyMessage request;
      request.set_info("I'm alive from: " + std::to_string(id_));
      request.set_port(port);
      escher::PayloadReadyResponse response;
      grpc::Status status = stub_->PayloadReady(&context, request, &response);
      if (not status.ok()) {
        Logger::error("Error while sending \"sendPayloadReady\": " + status.error_message());
        exit(-54);
      }
      Logger::info("Received " + response.info());
    }

    void escherCommunication() {
      grpc::ClientContext context;
      escher::EscherMessage message;
      escher::EscherMessageResponse response;
      grpc::Status status = stub_->EscherCommunication(&context, message, &response);
    }

  private:
    std::unique_ptr<escher::EscherletPayload::Stub> stub_;
    int                                             id_;
    int                                             port_;
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

void sendEscherCommunication(EscherletPayloadClient* client) {
  client->escherCommunication();
}

void mainLoop(EscherletPayloadClient* client) {
  sendEscherCommunication(client);
}

}

int main(int argc, char **argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  const char* const short_opts = "pi:h::";
  const option long_opts[] = {
    {"port",  required_argument,  0,        'p'},
    {"id",    required_argument,  0,        'i'},
    {"help" , no_argument,        nullptr,  'h'},
    {nullptr, no_argument,        nullptr,  0}
  };

  int masterPort = -1;
  int id = -1;

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
      case 'h':
      default:
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

  std::string logFileName("escher-app-" + std::to_string(id) + ".log");
  std::filesystem::path logFilePath(logFileName);
  logFile.open(logFilePath);
  Logger::info("########################################################");
  Logger::info(logFileName);
  Logger::info("########################################################");

  int localPort = getAvailablePort();
  std::string address("localhost");
  address += ":" + std::to_string(masterPort);
  Logger::info("creating local client");
  EscherletPayloadClient client(
    grpc::CreateChannel(address, grpc::InsecureChannelCredentials()),
    id,
    localPort
  );

  Logger::info("contacting escherlet on " + address + " and sending ready message");
  //I'm alive send message
  client.sendPayloadReady(localPort);

  //std::thread mainLoopThread(&mainLoop, &client);
  //mainLoopThread.join();

  //create and run local server
  Logger::info("starting server on " + std::to_string(localPort));
  std::string serverAddress("localhost:");
  serverAddress += std::to_string(localPort);
  EscherPayloadService service;
  grpc::ServerBuilder builder;
  builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  { 
    std::ostringstream message;
    message << "Server listening on " << serverAddress;
    Logger::info(message.str());
  }
  server->Wait(); //blocking

  //we get there if EscherPayloadService was terminated
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
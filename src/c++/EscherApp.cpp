#include <iostream>
#include <getopt.h>

#include <grpcpp/create_channel.h>
#include <grpcpp/server_builder.h>

#include "escher_payload.grpc.pb.h"

namespace {

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

    grpc::Status EscherCommunication(
      grpc::ServerContext* context,
      const escher::EscherMessage* message,
      escher::EscherMessageResponse* response) override {
        std::cout << "Escher Message received: "
          << "origin=" << message->origin()
          << ", key=" << message->key()
          << ", value=" << message->value()
          << std::endl;
        return grpc::Status::OK;
      }
};

void runServer() {
  std::string server_address("0.0.0.0:50051");
  EscherPayloadService service;
  grpc::ServerBuilder builder;
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

class EscherPayloadClient {
  public:
    EscherPayloadClient(std::shared_ptr<grpc::Channel> channel)
      : stub_(escher::EscherJob::NewStub(channel))
    {}

    void sendPayloadReady(int port) {
      grpc::ClientContext context;
      escher::PayloadReadyMessage request;
      request.set_info("I'm alive", port);
      escher::PayloadReadyResponse response;
      grpc::Status status = stub_->PayloadReady(&context, request, &response);
    }

  private:
    std::unique_ptr<escher::EscherJob::Stub> stub_;
};

int getAvailablePort() {
  //FIXME
  return 0;
}

void run(int masterPort, int payloadID) {
  std::string address("localhost");
  address += address + ":" + std::to_string(masterPort);
  
  //create local server


  EscherPayloadClient client(
    grpc::CreateChannel(address, grpc::InsecureChannelCredentials())
  );

  int port = getAvailablePort();
  //I'm alive send message
  client.sendPayloadReady(port);
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

  int port = -1;
  int id = -1;

  while (true) {
    const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
    if (opt == -1) {
      break;
    }

    switch (opt) {
      case 'p':
        port = std::stoi(optarg);
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

  if (port == -1) {
    std::cout << "port argument is mandatory" << std::endl;
    printHelp();
  }
  if (id == -1) {
    std::cout << "id argument is mandatory" << std::endl;
    printHelp();
  }

  //FIXME
  //create log file
  //

  run(port, id);

  // Optional:  Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}

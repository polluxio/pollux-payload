#include <iostream>
#include <getopt.h>

#include <grpcpp/create_channel.h>

#include "escher_payload.pb.h"

namespace {

void printHelp() {
  std::cout
    << "--port <p>: Escher master port" << std::endl
    << "--id <id>:  Escher payload id" << std::endl
    << "--help:     Displays this help" << std::endl;
  exit(1);
}

class EscherClient {
};

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

  {
    //I'm alive send message
    std::string address("localhost");
    address += address + ":" + std::to_string(port);
    grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
  }

  // Optional:  Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();

  return 0;
}

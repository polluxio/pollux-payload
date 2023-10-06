// SPDX-FileCopyrightText: 2023 Pollux authors <https://github.com/polluxio/pollux-payload/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "PolluxMethods.h"

#include <future>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>
#include "spdlog/sinks/basic_file_sink.h" // support for basic file logging

#include "pollux_payload.grpc.pb.h"

#include "ZebulonPayloadClient.h"
#include "PolluxPayload.h"
#include "PolluxPayloadException.h"

namespace {

std::promise<void> shutdownRequested;

class PolluxPayloadService final : public pollux::PolluxPayload::Service {
  public:
    PolluxPayloadService() = delete;
    PolluxPayloadService(const PolluxPayloadService&) = delete;
    PolluxPayloadService(ZebulonPayloadClient* client, PolluxPayload* payload):
      polluxPayLoad_(payload),
      zebulonClient_(client) {}
    grpc::Status Terminate(
      grpc::ServerContext* context,
      const pollux::PayloadTerminateMessage* messsage, 
      pollux::EmptyResponse* response) override {
      spdlog::info("Received terminate");
      shutdownRequested.set_value();
      return grpc::Status::OK;
    }

    grpc::Status Start(
      grpc::ServerContext* context,
      const pollux::PayloadStartMessage* message, 
      pollux::PolluxControlResponse* response) override {
      spdlog::info("Start payload received");
      try {
        polluxPayLoad_->setControl(message->control());
        polluxPayLoad_->init(zebulonClient_);
        std::thread mainLoopThread(&PolluxPayload::loop, polluxPayLoad_, zebulonClient_);
        mainLoopThread.detach();
      } catch (const PolluxPayloadException& e) {
        response->set_error(e.getReason());
        return grpc::Status::OK;
      }
      response->set_info("Payload has been started");
      return grpc::Status::OK;
    }

    grpc::Status Iterate(
      grpc::ServerContext* context,
      const pollux::PayloadIterateMessage* message, 
      pollux::PolluxControlResponse* response) override {
      spdlog::info("Iterate payload received, iteration: {}", message->iteration());
      try {
        std::thread mainLoopThread(&PolluxPayload::loop, polluxPayLoad_, zebulonClient_);
        mainLoopThread.detach();
      } catch (const PolluxPayloadException& e) {
        response->set_error(e.getReason());
        return grpc::Status::OK;
      }
      response->set_info("Payload next iteration");
      return grpc::Status::OK;
    }

    grpc::Status Transmit(
      grpc::ServerContext* context,
      const pollux::PolluxMessage* message,
      pollux::PolluxMessageResponse* response) override {
      spdlog::debug("Pollux Transmission received from zebulon");
      polluxPayLoad_->transmit(message);
      response->set_info("Transmit understood");
      return grpc::Status::OK;
    }
    void setServer(grpc::Server* server) {
      server_ = server;
    }
  private:
    ZebulonPayloadClient* zebulonClient_  {nullptr};
    grpc::Server*         server_         {nullptr};
    PolluxPayload*        polluxPayLoad_;
};

}

int Pollux::Main(int argc, char** argv, PolluxPayload* polluxPayload) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  argparse::ArgumentParser program("polluxapp");

  program.add_argument("-p", "--port")
    .scan<'d', int>()
    .required()
    .help("imposed port");
  program.add_argument("-i", "--id")
    .scan<'d', int>()
    .required()
    .help("local id");
  program.add_argument("-v", "--verbosity")
    .required()
    .help("verbosity: info/debug/trace (default:info)")
    .default_value(std::string("info"));
  program.add_argument("-t", "--zebulon_ip")
    .help("impose zebulon ip");

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  int zebulonPort = program.get<int>("--port");
  int id = program.get<int>("--id");
  std::string zebulonIP;
  if (auto zebulonIPOption = program.present("--zebulon_ip")) {
    zebulonIP = *zebulonIPOption;
  }

  std::string logFileName(polluxPayload->getName() + "-" + std::to_string(id) + ".log");
  auto myLogger = spdlog::basic_logger_mt("pollux_logger", logFileName.c_str());
  spdlog::flush_on(spdlog::level::info);
  spdlog::set_default_logger(myLogger);
  spdlog::flush_every(std::chrono::seconds(3));
  spdlog::info("########################################################");
  spdlog::info(logFileName);
  spdlog::info("########################################################");

  std::string verbosity = program.get<std::string>("--verbosity");
  if (verbosity == "debug") {
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("set log level to debug");
  } else if (verbosity == "trace") {
    spdlog::set_level(spdlog::level::trace);
    spdlog::info("set log level to trace");
  } else {
    if (verbosity != "info") {
      spdlog::warn("unrecognized verbose level: {}, forcing to \"info\"", verbosity);
    } else {
      spdlog::info("set log level to info");
    }
    spdlog::set_level(spdlog::level::info);
  }

  {
    std::ostringstream stream;
    for (int i=0; i<argc; i++) {
      stream << argv[i] << " ";
    }
    spdlog::info("Command line: {}", stream.str());
  }


  try {
    int localID = id;

    //ServerAddress serverAddress = getAvailableAddress();
    ZebulonPayloadClient* zebulonClient = nullptr;

    std::string zebulonAddress;
    std::string localServerAddress;
    if (not zebulonIP.empty()) {
      zebulonAddress = zebulonIP;
      localServerAddress = zebulonIP + ":0";
    } else {
      zebulonAddress = "localhost";
      localServerAddress = "localhost:0";
    }
    zebulonAddress += ":" + std::to_string(zebulonPort);

    spdlog::info("creating local client");
    zebulonClient = new ZebulonPayloadClient(
      grpc::CreateChannel(zebulonAddress, grpc::InsecureChannelCredentials()),
      localID
    );

    spdlog::info("starting server on " + localServerAddress);
    polluxPayload->setLocalID(localID);
    PolluxPayloadService service(zebulonClient, polluxPayload);
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
    service.setServer(server.get());
    spdlog::info("starting server on {}", std::to_string(serverPort));
    if (not server.get()) {
      std::ostringstream message;
      message << "GRPC Server could not be started on " << std::to_string(serverPort);
      throw PolluxPayloadException(message.str());
    }

    spdlog::info("contacting zebulon on {} and sending ready message", zebulonAddress);
    //I'm alive send message
    zebulonClient->sendPayloadReady(serverPort);

    auto serverWait = [&]() {
      //create and run local server
      spdlog::info("Server listening on {}", std::to_string(serverPort));
      server->Wait(); //blocking
    };

    std::thread serverThread(serverWait);

    auto f = shutdownRequested.get_future();
    f.wait();
    server->Shutdown();
    serverThread.join();

    //we get there if PolluxPayloadService was terminated
    spdlog::info("Server {} terminated", std::to_string(serverPort));

    delete zebulonClient;

    // Optional:  Delete all global objects allocated by libprotobuf.
    google::protobuf::ShutdownProtobufLibrary();
  } catch (PolluxPayloadException& e) {
    spdlog::error("PolluxPayloadException: {}", e.getReason());
  } catch (std::exception& e) {
    spdlog::error(e.what());
  }
  spdlog::info("End of {}", logFileName);
  return 0;
}

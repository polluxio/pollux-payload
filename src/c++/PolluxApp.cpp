#include <argparse/argparse.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <numeric>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h" // support for basic file logging

#include <grpcpp/grpcpp.h>

#include "pollux_payload.grpc.pb.h"

#include "ZebulonPayloadClient.h"
#include "PolluxAppException.h"

namespace {

class UserPayLoad {
  public:
    UserPayLoad(int localID): localID_(localID) {}
    UserPayLoad(const UserPayLoad&) = delete;

    void setControl(const pollux::PolluxControl& control) {
      control_ = control;
      for (auto id: control_.partids()) {
        if (id != localID_) {
          otherIDs_.push_back(id);
        }
      }
    }

    void loop(ZebulonPayloadClient* client) {
      spdlog::info("Main loop started iteration: {} synchronized: {}", iteration_, control_.synchronized());
      int nbMessages = 0;
      while (1) { //4x communication
        using namespace std::chrono_literals;
        //use more random sleep
        std::this_thread::sleep_for(5000ms);
        int pick = rand() % otherIDs_.size();
        int id = otherIDs_[pick];
        if (reporting_) {
          spdlog::info("Reporting {}", nbMessages);
          client->polluxReport(id, "key", "value");
        }
        spdlog::info("Message {}", nbMessages++);
        client->polluxCommunication(id, "key", "value");
        if (control_.synchronized() and nbMessages > 4) {
          break;
        }
      }
      if (control_.synchronized()) {
        if (iteration_ > 5) {
          client->sendPayloadLoopEnd(iteration_++);
        } else {
          client->sendPayloadLoopReadyForNextIteration(iteration_++);
        }
      }
    }

  private:
    int                     localID_;
    std::vector<int>        otherIDs_;
    pollux::PolluxControl   control_;
    bool                    reporting_ = true;
    int                     iteration_ = 0;
};

class PolluxPayloadService final : public pollux::PolluxPayload::Service {
  public:
    PolluxPayloadService() = delete;
    PolluxPayloadService(const PolluxPayloadService&) = delete;
    PolluxPayloadService(ZebulonPayloadClient* client, UserPayLoad* payload):
      userPayLoad_(payload),
      zebulonClient_(client) {}
    grpc::Status Terminate(
      grpc::ServerContext* context,
      const pollux::PayloadTerminateMessage* messsage, 
      pollux::EmptyResponse* response) override {
      spdlog::info("Received terminate");
      //Terminate local app
      //should the server be gracefully shutdown ??
      if (server_) {
        spdlog::info("Shutting down server");
        server_->Shutdown();
      }
      return grpc::Status::OK;
    }

    grpc::Status Start(
      grpc::ServerContext* context,
      const pollux::PayloadStartMessage* message, 
      pollux::PolluxStandardResponse* response) override {
      spdlog::info("Start payload received");
      userPayLoad_->setControl(message->control());

      std::thread mainLoopThread(&UserPayLoad::loop, userPayLoad_, zebulonClient_);
      mainLoopThread.detach();
      response->set_info("Payload has been started");
      return grpc::Status::OK;
    }

    grpc::Status Iterate(
      grpc::ServerContext* context,
      const pollux::PayloadIterateMessage* message, 
      pollux::PolluxStandardResponse* response) override {
      spdlog::info("Iterate payload received, iteration: {}", message->iteration());
      std::thread mainLoopThread(&UserPayLoad::loop, userPayLoad_, zebulonClient_);
      mainLoopThread.detach();
      response->set_info("Payload next iteration");
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
      spdlog::info("{}", logMessage.str());
      response->set_info(logMessage.str() + " understood");
      return grpc::Status::OK;
    }

    void setServer(grpc::Server* server) {
      server_ = server;
    }
  private:
    ZebulonPayloadClient* zebulonClient_  {nullptr};
    grpc::Server*         server_         {nullptr};
    UserPayLoad*          userPayLoad_;
};

}

int main(int argc, char **argv) {
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

  std::string logFileName("pollux-app-" + std::to_string(id) + ".log");
  auto myLogger = spdlog::basic_logger_mt("pollux_logger", logFileName.c_str());
  spdlog::flush_on(spdlog::level::info);
  spdlog::set_default_logger(myLogger);
  spdlog::flush_every(std::chrono::seconds(3));
  spdlog::info("########################################################");
  spdlog::info(logFileName);
  spdlog::info("########################################################");

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
    UserPayLoad userPayLoad(localID);
    PolluxPayloadService service(zebulonClient, &userPayLoad);
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
      throw PolluxAppException(message.str());
    }

    spdlog::info("contacting zebulon on {} and sending ready message", zebulonAddress);
    //I'm alive send message
    zebulonClient->sendPayloadReady(serverPort);

    //create and run local server
    spdlog::info("Server listening on {}", std::to_string(serverPort));
    server->Wait(); //blocking

    //we get there if PolluxPayloadService was terminated
    spdlog::info("Server {} terminated", std::to_string(serverPort));

    delete zebulonClient;

    // Optional:  Delete all global objects allocated by libprotobuf.
    google::protobuf::ShutdownProtobufLibrary();
  } catch (PolluxAppException& e) {
    spdlog::error("PolluxAppException: {}", e.getReason());
  } catch (std::exception& e) {
    spdlog::error(e.what());
  }
  spdlog::info("End of {}", logFileName);
  return 0;
}

#include "ZebulonPayloadClient.h"

#include "spdlog/spdlog.h"

ZebulonPayloadClient::ZebulonPayloadClient(std::shared_ptr<grpc::Channel> channel, int id):
  stub_(pollux::ZebulonPayload::NewStub(channel)),
  id_(id)
{}

void ZebulonPayloadClient::sendPayloadReady(uint16_t port) {
  grpc::ClientContext context;
  pollux::PolluxVersion* version = new pollux::PolluxVersion();
  version->set_version(pollux::PolluxVersion_Version::PolluxVersion_Version_CURRENT);
  pollux::PayloadReadyMessage request;
  request.set_info("I'm alive from: " + std::to_string(id_));
  request.set_port(port);
  request.set_allocated_version(version);
  pollux::PolluxStandardResponse response;
  grpc::Status status = stub_->PayloadReady(&context, request, &response);
  if (not status.ok()) {
    spdlog::error("Error while sending \"sendPayloadReady\": {}", status.error_message());
    exit(-54);
  }
  spdlog::info("Response from Zebulon to PayloadReady: {}", response.info());
}

void ZebulonPayloadClient::sendPayloadLoopReadyForNextIteration(int iteration) {
  grpc::ClientContext context;
  pollux::PayloadLoopMessage request;
  request.set_iteration(iteration);
  pollux::PolluxStandardResponse response;
  grpc::Status status = stub_->PayloadLoopReadyForNextIteration(&context, request, &response);
  spdlog::info("Sending PayloadLoopReadyForNextIteration");
  if (not status.ok()) {
    spdlog::error("Error while sending \"sendPayloadLoopReadyForNextIteration\": {}", status.error_message());
    exit(-54);
  }
  spdlog::info("Response from Zebulon to PayloadLoopReadyForNextIteration: {}", response.info());
}

void ZebulonPayloadClient::sendPayloadLoopEnd(int iteration) {
  grpc::ClientContext context;
  pollux::PayloadLoopMessage request;
  request.set_iteration(iteration);
  pollux::PolluxStandardResponse response;
  grpc::Status status = stub_->PayloadLoopEnd(&context, request, &response);
  spdlog::info("Sending PayloadEnd");
  if (not status.ok()) {
    spdlog::error("Error while sending \"sendPayloadLoopEnd\": {}", status.error_message());
    exit(-54);
  }
  spdlog::info("Response from Zebulon to PayloadEnd: ", response.info());
}

void ZebulonPayloadClient::polluxCommunication(int id, const std::string& key, const std::string& value) {
  grpc::ClientContext context;
  pollux::PolluxMapMessage message;
  message.set_origin(id_);
  message.add_destinations(id);
  pollux::PolluxMessageValue messageValue;
  messageValue.set_strvalue(value);
  (*message.mutable_map())[key] = messageValue;

  pollux::PolluxMessageResponse response;
  grpc::Status status = stub_->PolluxCommunication(&context, message, &response);
  spdlog::info("PolluxCommunication::Response: {}", response.info());
}

std::string ZebulonPayloadClient::getString() const {
  return "ZebulonPayloadClient id: " + std::to_string(id_);
}

#include "ZebulonPayloadClient.h"

#include <plog/Log.h>

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
    PLOG_ERROR << "Error while sending \"sendPayloadReady\": " << status.error_message();
    exit(-54);
  }
  PLOG_INFO << "Response from Zebulon to PayloadReady: " << response.info();
}

void ZebulonPayloadClient::polluxCommunication(int id, const std::string& key, const std::string& value) {
  grpc::ClientContext context;
  pollux::PolluxMessage message;
  message.set_origin(id_);
  message.add_destinations(id);
  message.set_key(key);
  message.set_value(value);
  pollux::PolluxMessageResponse response;
  grpc::Status status = stub_->PolluxCommunication(&context, message, &response);
  PLOG_INFO << "PolluxCommunication::Response: " << response.info();
}

std::string ZebulonPayloadClient::getString() const {
  return "ZebulonPayloadClient id: " + std::to_string(id_);
}

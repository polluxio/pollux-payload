// SPDX-FileCopyrightText: 2023 Pollux authors <https://github.com/polluxio/pollux-payload/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "ZebulonPayloadClient.h"

#include "spdlog/spdlog.h"

namespace {

void transmit(
  const ZebulonPayloadClient::Destinations& destinations,
  int origin,
  pollux::ZebulonPayload::Stub* stub,
  const std::string& key,
  pollux::PolluxMessage& message) {
  const auto start{std::chrono::steady_clock::now()};
  message.set_origin(origin);
  message.set_key(key);
  for (auto destination: destinations) {
    message.add_destinations(destination);
  }
  grpc::ClientContext context;
  pollux::PolluxMessageResponse response;
  grpc::Status status = stub->Transmit(&context, message, &response);
  if (not status.ok()) {
    spdlog::error("Error while \"transmit\": {}", status.error_message());
    exit(-54);
  }
  const auto end{std::chrono::steady_clock::now()};
  const std::chrono::duration<double> elapsed_seconds{end - start};
  spdlog::debug("Transmit::Response: {} in {:.6f} seconds", response.info(), elapsed_seconds.count());
}

ZebulonPayloadClient::NodeStatus grpcNodeStatusToNodeStatus(pollux::NodeStatusResponse_NodeStatus grpcStatus) {
  switch (grpcStatus) {
    case pollux::NodeStatusResponse_NodeStatus_UNKNOWN:
      return ZebulonPayloadClient::NodeStatus::Unknown;
    case pollux::NodeStatusResponse_NodeStatus_RUNNING:
      return ZebulonPayloadClient::NodeStatus::Running;
    case pollux::NodeStatusResponse_NodeStatus_DONE:
      return ZebulonPayloadClient::NodeStatus::Done;
    case pollux::NodeStatusResponse_NodeStatus_TERMINATED:
      return ZebulonPayloadClient::NodeStatus::Terminated;
    case pollux::NodeStatusResponse_NodeStatus_FAILURE:
      return ZebulonPayloadClient::NodeStatus::Failure;
    default:
      break;
  }
  return ZebulonPayloadClient::NodeStatus::InternalFailure;
}

}

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
  spdlog::debug("Sending Payload Ready with port {} and GRPC schema version {}",
      port, pollux::PolluxVersion_Version::PolluxVersion_Version_CURRENT);
  pollux::PolluxStandardResponse response;
  grpc::Status status = stub_->PayloadReady(&context, request, &response);
  if (not status.ok()) {
    spdlog::error("Error while sending \"sendPayloadReady\": {}", status.error_message());
    exit(-54);
  }
  spdlog::debug("Response from Zebulon to PayloadReady: {}", response.info());
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
  spdlog::debug("Response from Zebulon to PayloadLoopReadyForNextIteration: {}", response.info());
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
  spdlog::debug("Response from Zebulon to PayloadEnd: ", response.info());
}

void ZebulonPayloadClient::transmit(const Destinations& destinations, const std::string& key, const std::string& value) {
  pollux::PolluxMessage message;
  message.set_strvalue(value);
  ::transmit(destinations, id_, stub_.get(), key, message);
}

void ZebulonPayloadClient::transmit(int id, const std::string& key, const std::string& value) {
  transmit(Destinations({id}), key, value);
}

void ZebulonPayloadClient::transmit(const std::string& key, const std::string& value) {
  transmit(Destinations(), key, value);
}

void ZebulonPayloadClient::transmit(const Destinations& destinations, const std::string& key, int64_t value) {
  pollux::PolluxMessage message;
  message.set_int64value(value);
  ::transmit(destinations, id_, stub_.get(), key, message);
}

void ZebulonPayloadClient::transmit(int id, const std::string& key, int64_t value) {
  transmit(Destinations({id}), key, value);
}

void ZebulonPayloadClient::transmit(const std::string& key, int64_t value) {
  transmit(Destinations(), key, value);
}

void ZebulonPayloadClient::transmit(const Destinations& destinations, const std::string& key, const Int64Array& values) {
  pollux::PolluxMessage message;
  auto int64Array = message.mutable_int64arrayvalue();
  for (auto value: values) {
    int64Array->add_values(value);
  }
  ::transmit(destinations, id_, stub_.get(), key, message);
}

void ZebulonPayloadClient::transmit(int id, const std::string& key, const Int64Array& values) {
  transmit(Destinations({id}), key, values);
}

void ZebulonPayloadClient::transmit(const std::string& key, const Int64Array& values) {
  transmit(Destinations(), key, values);
}

void ZebulonPayloadClient::transmit(const Destinations& destinations, const std::string& key, const DoubleArray& values) {
  pollux::PolluxMessage message;
  auto doubleArray = message.mutable_doublearrayvalue();
  for (auto value: values) {
    doubleArray->add_values(value);
  }
  ::transmit(destinations, id_, stub_.get(), key, message);
}

void ZebulonPayloadClient::transmit(int id, const std::string& key, const DoubleArray& values) {
  transmit(Destinations({id}), key, values);
}

void ZebulonPayloadClient::transmit(const std::string& key, const DoubleArray& values) {
  transmit(Destinations(), key, values);
}

void ZebulonPayloadClient::polluxLog(const std::string& key, const std::string& value) {
  grpc::ClientContext context;
  pollux::PolluxLogMessage message;
  message.set_origin(id_);
  (*message.mutable_map())[key] = value;

  pollux::PolluxStandardResponse response;
  grpc::Status status = stub_->PolluxLog(&context, message, &response);
  if (not status.ok()) {
    spdlog::error("Error while sending \"polluxLog\": {}", status.error_message());
    exit(-54);
  }
  spdlog::debug("PolluxLog: {}", response.info());
}

void ZebulonPayloadClient::polluxReport(const std::string& key, const std::string& value) {
  grpc::ClientContext context;
  pollux::PolluxReportMessage message;
  message.set_origin(id_);
  (*message.mutable_map())[key] = value;

  pollux::PolluxStandardResponse response;
  grpc::Status status = stub_->PolluxReport(&context, message, &response);
  if (not status.ok()) {
    spdlog::error("Error while sending \"polluxReport\": {}", status.error_message());
    exit(-54);
  }
  spdlog::debug("PolluxReport: {}", response.info());
}

ZebulonPayloadClient::NodeStatus::NodeStatus(const NodeStatusEnum& nodeStatusEnum):
  nodeStatusEnum_(nodeStatusEnum) 
{}

std::string ZebulonPayloadClient::NodeStatus::getString() const {
  switch (nodeStatusEnum_) {
    case NodeStatus::Unknown: return "Unknown";
    case NodeStatus::Running: return "Running";
    case NodeStatus::Done: return "Done";
    case NodeStatus::Terminated: return "Terminated";
    case NodeStatus::Failure: return "Failure";
    case NodeStatus::InternalFailure: return "InternalFailure";
  }
  return "EnumError";
}

ZebulonPayloadClient::NodeStatus ZebulonPayloadClient::getNodeStatus(unsigned nodeID) const {
  grpc::ClientContext context;
  pollux::NodeStatusMessage message;
  message.set_nodeid(nodeID);

  pollux::NodeStatusResponse response;
  grpc::Status status = stub_->GetNodeStatus(&context, message, &response);
  if (not status.ok()) {
    spdlog::error("Error while sending \"getNodeStatus\": {}", status.error_message());
    exit(-54);
  }
  ZebulonPayloadClient::NodeStatus nodeStatus = grpcNodeStatusToNodeStatus(response.status());
  spdlog::debug("getNodeStatus: {}", nodeStatus.getString());
  return nodeStatus;
}

std::string ZebulonPayloadClient::getString() const {
  return "ZebulonPayloadClient id: " + std::to_string(id_);
}

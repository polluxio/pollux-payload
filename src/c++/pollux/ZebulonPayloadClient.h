// SPDX-FileCopyrightText: 2023 Pollux authors <https://github.com/polluxio/pollux-payload/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#ifndef __ZEBULON_PAYLOAD_CLIENT_H_
#define __ZEBULON_PAYLOAD_CLIENT_H_

#include <memory>

#include <grpcpp/grpcpp.h>
#include "pollux_payload.grpc.pb.h"

class ZebulonPayloadClient {
  public:
    ZebulonPayloadClient(std::shared_ptr<grpc::Channel> channel, int id);

    void sendPayloadReady(uint16_t port);
    void sendPayloadLoopReadyForNextIteration(int iteration);
    void sendPayloadLoopEnd(int iteration);
    
    //send communication to outside world
    using Destinations = std::vector<int>;
    void transmit(const Destinations& destinations, const std::string& key, const std::string& value);
    void transmit(int destination, const std::string& key, const std::string& value);
    void transmit(const std::string& key, const std::string& value);


    void transmit(const Destinations& destinations, const std::string& key, int64_t value);
    void transmit(int destination, const std::string& key, int64_t value);
    void transmit(const std::string& key, int64_t value);

    using Int64Array = std::vector<int64_t>;
    void transmit(const Destinations& destinations, const std::string& key, const Int64Array& values);
    void transmit(int destination, const std::string& key, const Int64Array& values);
    void transmit(const std::string& key, const Int64Array& values);

    using DoubleArray = std::vector<double>;
    void transmit(const Destinations& destinations, const std::string& key, const DoubleArray& values);
    void transmit(int destination, const std::string& key, const DoubleArray& values);
    void transmit(const std::string& key, const DoubleArray& values);

    class NodeStatus {
      public:
        enum NodeStatusEnum {
          Unknown,        // The node/payload is unknown: no node with this ID exists
          Running,        // The node/payload is running
          Done,           // The node/payload is done: next status Terminated
          Terminated,     // The node/payload has been terminated
          Failure,        // The node/payload has failed
          InternalFailure // Internal Failure during query

        };
        NodeStatus(const NodeStatusEnum& nodeStatusEnum);
        NodeStatus(const NodeStatus& direction) = default;
        operator const NodeStatusEnum&() const {return nodeStatusEnum_;}
        std::string getString() const;
        private:
          NodeStatusEnum  nodeStatusEnum_;
    };
    NodeStatus getNodeStatus(unsigned nodeID) const;

    void polluxLog(const std::string& key, const std::string& value);
    void polluxReport(const std::string& key, const std::string& value);
    std::string getString() const;

  private:
    std::unique_ptr<pollux::ZebulonPayload::Stub> stub_;
    int                                             id_;
};

#endif // __ZEBULON_PAYLOAD_CLIENT_H_

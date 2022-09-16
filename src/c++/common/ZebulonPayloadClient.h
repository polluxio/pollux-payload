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
    void polluxCommunication(int id, const std::string& key, const std::string& value);
    std::string getString() const;

  private:
    std::unique_ptr<pollux::ZebulonPayload::Stub> stub_;
    int                                             id_;
};

#endif // __ZEBULON_PAYLOAD_CLIENT_H_

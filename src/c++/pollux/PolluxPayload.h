#ifndef __POLLUX_PAYLOAD_H_
#define __POLLUX_PAYLOAD_H_

#include "ZebulonPayloadClient.h"

class PolluxPayload {
  public:
    PolluxPayload()=default;
    PolluxPayload(const PolluxPayload&)=delete;

    virtual void loop(ZebulonPayloadClient* client) =0;
    void setLocalID(int localID) { localID_ = localID; }
    bool isSynchronized() const { return control_.synchronized(); } 
    std::vector<int> getOtherIDs() const { return otherIDs_; }

    void setControl(const pollux::PolluxControl& control) {
      control_ = control;
      for (auto id: control_.partids()) {
        if (id != localID_) {
          otherIDs_.push_back(id);
        }
      }
    }

  private:
    int                     localID_;
    std::vector<int>        otherIDs_;
    pollux::PolluxControl   control_;
};

#endif /* __POLLUX_PAYLOAD_H_ */

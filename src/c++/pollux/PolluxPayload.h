#ifndef __POLLUX_PAYLOAD_H_
#define __POLLUX_PAYLOAD_H_

#include "ZebulonPayloadClient.h"

class PolluxPayload {
  public:
    PolluxPayload()=default;
    PolluxPayload(const PolluxPayload&)=delete;

    enum UserOptionType { LONG, STRING };
    using UserOptionValue = std::variant<long, std::string>;
    using UserOptions = std::map<std::string, UserOptionValue>;

    void setLocalID(int localID) { localID_ = localID; }
    int getLocalID() const { return localID_; }
    std::vector<int> getOtherIDs() const { return otherIDs_; }

    UserOptions getUserOptions() const { return userOptions_; }
    //returns nullptr if does not exist
    //if found: returns ptr to option
    UserOptionValue* getUserOptionValue(const std::string& name);

    bool isSynchronized() const { return control_.synchronized(); } 

    //Implemented by final user
    virtual void loop(ZebulonPayloadClient* client) =0;

    void setControl(const pollux::PolluxControl& control);

  private:
    int                     localID_      {-1};
    std::vector<int>        otherIDs_     {};
    pollux::PolluxControl   control_      {};
    UserOptions             userOptions_  {};
};

#endif /* __POLLUX_PAYLOAD_H_ */

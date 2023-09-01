#include "PolluxPayload.h"

void PolluxPayload::setControl(const pollux::PolluxControl& control) {
  control_ = control;
  for (auto id: control_.partids()) {
    if (id != localID_) {
      otherIDs_.push_back(id);
    }
  }
  for (auto const& [name, value]: control.useroptions()) {
    //check for collision
    if (value.has_strvalue()) {
        userOptions_[name] = value.strvalue(); 
    } else if (value.has_int64value()) {
        userOptions_[name] = value.int64value(); 
    }
  }
}
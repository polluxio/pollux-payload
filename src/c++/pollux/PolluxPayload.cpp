// SPDX-FileCopyrightText: 2023 Pollux authors <https://github.com/polluxio/pollux-payload/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "PolluxPayload.h"

#include "PolluxPayloadException.h"

void PolluxPayload::setControl(const pollux::PolluxControl& control) {
  control_ = control;
  for (auto id: control_.partids()) {
    if (id != localID_) {
      otherIDs_.push_back(id);
    }
  }
  for (auto const& [name, value]: control.useroptions()) {
    //check for collision
    switch(value.value_case()) {
      case pollux::PolluxUserOptionValue::kStrValue:
        userOptions_[name] = value.strvalue(); 
        break;
      case pollux::PolluxUserOptionValue::kBoolValue:
        userOptions_[name] = value.boolvalue(); 
        break;
      case pollux::PolluxUserOptionValue::kInt64Value:
        userOptions_[name] = value.int64value(); 
        break;
      default:
        throw PolluxPayloadException("Unset user option value");
    }
  }
}

PolluxPayload::UserOptionValue* PolluxPayload::getUserOptionValue(const std::string& name) {
  auto uoit = userOptions_.find(name);
  if (uoit != userOptions_.end()) {
    return &uoit->second;
  }
  return nullptr;
}

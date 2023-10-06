// SPDX-FileCopyrightText: 2023 Pollux authors <https://github.com/polluxio/pollux-payload/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#ifndef __POLLUX_PAYLOAD_EXCEPTION_H_
#define __POLLUX_PAYLOAD_EXCEPTION_H_

class PolluxPayloadException: public std::exception {
  public:
    PolluxPayloadException() = delete;
    PolluxPayloadException(const PolluxPayloadException&) = default;

    PolluxPayloadException(const std::string& reason):
      std::exception(),
      reason_(reason)
    {}
    
    std::string getReason() const {
      return reason_;
    }

    const char* what() const noexcept override {
      return reason_.c_str();
    }

  private:
    const std::string reason_;
};

#endif /* __POLLUX_PAYLOAD_EXCEPTION_H_ */

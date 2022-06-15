#ifndef __POLLUX_APP_EXCEPTION_H_
#define __POLLUX_APP_EXCEPTION_H_

class PolluxAppException: public std::exception {
  public:
    PolluxAppException() = delete;
    PolluxAppException(const PolluxAppException&) = default;

    PolluxAppException(const std::string& reason):
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

#endif /* __POLLUX_APP_EXCEPTION_H_ */

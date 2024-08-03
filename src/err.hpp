#pragma once

#include <exception>
#include <string>
#include <utility>

namespace chat {

class StringException : public std::exception {
  private:
    std::string msg;

  public:
    StringException(std::string msg_) noexcept : msg(std::move(msg_)) {}

    [[nodiscard]] const char *what() const noexcept {
        return msg.c_str();
    }
};

} // namespace chat

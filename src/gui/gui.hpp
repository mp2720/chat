#pragma once

#include "io.hpp"
#include <exception>
#include <string>

namespace chat::gui {

class Exception : public std::exception {
    std::string str;

  public:
    inline Exception(std::string str_) : str(std::move(str_)){};
};

class Gui {
  private:
    Io io;

  public:
    void init();
    void loopUntilExit();
    void terminate();
};

} // namespace chat::gui

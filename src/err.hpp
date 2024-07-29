#pragma once

#include <string>

namespace chat {
namespace err {
    void init();
    [[noreturn]] void panic(std::string &&msg);
}
}
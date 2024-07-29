#include "log.hpp"

#include <boost/format.hpp>
#include <cassert>
#include <iomanip>
#include <mutex>

using namespace chat;

void Logger::log(Severity severity, const char *file, long line, std::string &&str) {
    assert(output && "output must be set");
    assert(filter && "filter must be set");

    if (!filter(severity, file, line, str))
        return;

    const char *severity_str, *severity_color_code;
    switch (severity) {
    case ERROR:
        severity_str = "E";
        severity_color_code = "31";
        break;
    case WARNING:
        severity_str = "W";
        severity_color_code = "33";
        break;
    case INFO:
        severity_str = "I";
        severity_color_code = "34";
        break;
    case VERBOSE:
        severity_str = "V";
        severity_color_code = "36";
        break;
    default:
        assert(false && "invalid severity");
    }

    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);

    {
        std::lock_guard lock(mutex);

        *output << "\e[1;" << severity_color_code << 'm' << severity_str << " ("
                << std::put_time(&tm, "%H:%M:%S") << ") ";

        if (file != nullptr)
            *output << '[' << file << ':' << line << "] ";

        *output << "\e[0m" << str << std::endl;
    }
}

void Logger::log(Severity severity, const char *file, long line, const boost::format &fmt) {
    log(severity, file, line, fmt.str());
}

Logger chat::global_logger;
#include "log.hpp"

#include <boost/format.hpp>
#include <cassert>
#include <iomanip>
#include <mutex>
#include <time.h> // localtime_r()

using namespace chat;

void Logger::log(Severity severity, const char *file, long line, std::string &&str) noexcept {
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

    struct tm m_time;
    time_t s_time = time(nullptr);

#ifdef CHAT_BUILD_TARGET_WINDOWS
    localtime_s(&m_time, &s_time);
#else
    localtime_r(&s_time, &m_time);
#endif

    {
        std::lock_guard lock(mutex);

        *output << "\e[1;" << severity_color_code << 'm' << severity_str << " ("
                << std::put_time(&m_time, "%H:%M:%S") << ") ";

        if (file != nullptr)
            *output << '[' << file << ':' << line << "] ";

        *output << "\e[0m" << str << std::endl;
    }
}

Logger chat::global_logger;

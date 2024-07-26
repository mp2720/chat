#pragma once

#include <boost/format.hpp>
#include <boost/format/format_fwd.hpp>
#include <functional>
#include <mutex>
#include <ostream>

namespace chat {

enum Severity { SEVERITY_ERROR, SEVERITY_WARNING, SEVERITY_INFO, SEVERITY_VERBOSE };

class Logger {
  public:
    using Filter =
        std::function<bool(Severity severity, const char *file, long line, const std::string &msg)>;

    void log(Severity severity, const char *file, long line, std::string &&str);

    void log(Severity severity, const char *file, long line, const boost::format &fmt);

    inline void setFilter(Filter filter) {
        this->filter = filter;
    }

    inline void setOutput(std::ostream *output) {
        this->output = output;
    }

  private:
    std::ostream *output = nullptr;
    std::mutex mutex;
    Filter filter;
};

extern Logger global_logger;

#define CHAT_LOGE(msg) chat::global_logger.log(chat::SEVERITY_ERROR, __FILE__, __LINE__, msg)
#define CHAT_LOGW(msg) chat::global_logger.log(chat::SEVERITY_WARNING, __FILE__, __LINE__, msg)
#define CHAT_LOGI(msg) chat::global_logger.log(chat::SEVERITY_INFO, __FILE__, __LINE__, msg)
#define CHAT_LOGV(msg) chat::global_logger.log(chat::SEVERITY_VERBOSE, __FILE__, __LINE__, msg)

} // namespace chat

#pragma once

#include <boost/format.hpp>
#include <boost/format/format_fwd.hpp>
#include <functional>
#include <mutex>
#include <ostream>

namespace chat {

class Logger {
  public:
    enum Severity { ERROR, WARNING, INFO, VERBOSE };

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

    inline std::ostream *getOutput() {
        return this->output;
    }

  private:
    std::ostream *output = nullptr;
    std::mutex mutex;
    Filter filter;
};

extern Logger global_logger;

#define CHAT_LOG_NO_FILE_LINE(sev, msg) chat::global_logger.log(sev, nullptr, 0, msg)
#define CHAT_LOG(sev, msg) chat::global_logger.log(sev, __FILE__, __LINE__, msg)

#define CHAT_LOGE(msg) CHAT_LOG(chat::Logger::ERROR, msg)
#define CHAT_LOGW(msg) CHAT_LOG(chat::Logger::WARNING, msg)
#define CHAT_LOGI(msg) CHAT_LOG(chat::Logger::INFO, msg)
#define CHAT_LOGV(msg) CHAT_LOG(chat::Logger::VERBOSE, msg)

} // namespace chat
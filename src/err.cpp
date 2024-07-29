#include "err.hpp"
#include "log.hpp"
#include <boost/stacktrace.hpp>
#include <boost/stacktrace/stacktrace.hpp>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <sstream>
#include <string>

using namespace chat;

[[noreturn]] void err::panic(std::string &&msg) {
    CHAT_LOGE("Panic:");
    CHAT_LOGE(std::move(msg));
    CHAT_LOGE("Stacktrace:");
    *::global_logger.getOutput() << boost::stacktrace::stacktrace();
    std::exit(EXIT_FAILURE);
}

[[noreturn]] void chat_err_signal_handler(int sig) {
    std::signal(sig, SIG_DFL);
    std::string msg;
    if (sig == SIGABRT) {
        msg = "(SIGABRT) Aborted";
    } else if (sig == SIGSEGV) {
        msg = "(SIGSEGV) Invalid memory access";
    } else if (sig == SIGILL) {
        msg = "(SIGILL) Invalid instruction";
    } else if (sig == SIGFPE) {
        msg = "(SIGFPE) Erroneous arithmetic operation";
    }
    err::panic(std::move(msg));
}

[[noreturn]] void chat_err_terminate_handler() {
    std::exception_ptr exptr = std::current_exception();
    if (exptr) {
        try {
            std::rethrow_exception(exptr);
        } catch (std::exception &ex) {
            std::stringstream ss;
            ss << "Terminate called after throwing an instance of '" << typeid(ex).name() << "':\n"
               << ex.what();
            err::panic(ss.str());
        }
    } else {
        err::panic("Terminate called without an active exception");
    }
}

[[noreturn]] void
err::_assert_fail(const char *expr, const char *file, const char *func, int line) {
    std::stringstream ss;
    ss << "Assertation '" << expr << "' failed, '" << func << "' in '" << file << ":" << line
       << "'";
    err::panic(ss.str());
}

void err::init() {
    std::signal(SIGABRT, chat_err_signal_handler);
    std::signal(SIGSEGV, chat_err_signal_handler);
    std::signal(SIGILL, chat_err_signal_handler);
    std::signal(SIGFPE, chat_err_signal_handler);
    std::set_terminate(chat_err_terminate_handler);
}
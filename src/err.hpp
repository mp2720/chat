#pragma once

#include <boost/current_function.hpp>
#include <string>

namespace chat {
namespace err {
void init();
[[noreturn]] void panic(std::string &&msg);
[[noreturn]] void _assert_fail(const char *expr, const char *file, const char *func, int line);
} // namespace err
} // namespace chat

#ifdef NDEBUG
#define CHAT_ASSERT(expr) ((void)0)
#else /* Not NDEBUG.  */
#define CHAT_ASSERT(expr)    \
    (static_cast<bool>(expr) \
         ? void(0)           \
         : chat::err::_assert_fail(#expr, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__))
#endif
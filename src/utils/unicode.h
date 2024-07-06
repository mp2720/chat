#pragma once

#include "utf8.h"

namespace chat_utils {

bool unicodeIsLineBreak(utf8::utfchar32_t c);

bool unicodeIsSpace(utf8::utfchar32_t c);

} // namespace chat_utils

#include "unicode.h"

using namespace chat_utils;

bool chat_utils::unicodeIsLineBreak(utf8::utfchar32_t c) {
    // https://en.wikipedia.org/wiki/Newline#Unicode
    return c == 0x000a    // LF
           || c == 0x000b // VT
           || c == 0x000c // FF
           || c == 0x0085 // NEL
           || c == 0x2028 // LS
           || c == 0x2029 // PS
        ;
}

bool chat_utils::unicodeIsSpace(utf8::utfchar32_t c) {
    // https://en.wikipedia.org/wiki/Whitespace_character#Unicode
    return c == 0x0009    // character tabulation
           || c == 0x0020 // space
           || c == 0x205f // medium mathematical space
           || c == 0x3000 // ideographic space
           || c == 0x180e // mongolian vowel separator
           || c == 0x200b // zero width space
           || c == 0x200c // zero width non-joiner
           || c == 0x200d // zero width joiner
           || c == 0x1680 || (0x2000 <= c && c <= 0x200a && c != 0x2007) // en quad
                                                                         // em quad
                                                                         // en space
                                                                         // em space
                                                                         // three-per-em space
                                                                         // four-per-em space
                                                                         // six-per-em space
                                                                         // punctuation space
                                                                         // thin space
                                                                         // hair space
        ;
}

#include "text.h"

#include "imgui.h"
#include "utils/unicode.h"
#include <algorithm>
#include <iostream>
#include <sstream>

using namespace chat_gui;

using chat_utils::unicodeIsLineBreak;
using chat_utils::unicodeIsSpace;
using std::string;
using std::vector;

WrappedText::WrappedText(std::string _str) : str(std::move(_str)) {}

CannotFitGlyphException::CannotFitGlyphException(uint32_t c, float max_width) {
    std::stringstream ss;
    ss << "Width of glyph for the unicode symbol 0x" << std::hex << c
       << " is bigger than max_width=" << max_width << '.';
    msg = ss.str();
}

const char *CannotFitGlyphException::what() const noexcept {
    return msg.c_str();
}

WrappedText::Line::Line(string::const_iterator _start) : start(_start) {}

WrappedText::Decorator::Decorator(const WrappedText &_text) : text(_text) {}

void WrappedText::wrapBigWord(
    string::const_iterator word_start,
    string::const_iterator word_end,
    float max_width
) {
    float width = 0;
    auto str_it = word_start;

    while (str_it != word_end) {
        auto prev_it = str_it;
        auto c = utf8::next(str_it, word_end);
        float char_width = ImGui::CalcTextSize(&*prev_it, &*str_it).x;

        if (width + char_width <= max_width) {
            width += char_width;
            continue;
        }

        if (char_width > max_width)
            throw CannotFitGlyphException(c, max_width);

        lines.back().width = width;
        lines.back().visible_end = prev_it;
        lines.push_back(Line(prev_it));
        width = char_width;
    }

    lines.back().width = width;
}

void WrappedText::cutSpaces(
    string::const_iterator spaces_start,
    string::const_iterator spaces_end,
    float max_width
) {
    auto str_it = spaces_start;

    while (str_it != spaces_end) {
        auto prev_it = str_it;
        utf8::next(str_it, spaces_end);
        float char_width = ImGui::CalcTextSize(&*prev_it, &*spaces_end).x;

        if (lines.back().width + char_width > max_width) {
            lines.back().width = max_width;
            lines.back().visible_end = prev_it;
            break;
        }

        lines.back().width += char_width;
    }
}

void WrappedText::calcLines(float max_width) {
    lines.clear();

    enum { WORD, SPACE, NEWLINE } state = WORD;
    string::iterator str_it = str.begin();
    string::iterator segment_start = str.begin();

    bool cont = true;

    lines.push_back(Line(str.begin()));

    while (cont) {
        string::iterator prev_it = str_it;

        utf8::utfchar32_t c;
        if (str_it != str.end()) {
            c = utf8::next(str_it, str.end());
        } else {
            c = 0;
            cont = false;
        }

        switch (state) {
        case WORD: {
            if (!(unicodeIsSpace(c) || unicodeIsLineBreak(c) || c == 0))
                break;

            float word_width = ImGui::CalcTextSize(&*segment_start, &*prev_it).x;
            if (word_width > max_width) {
                if (lines.back().width != 0) {
                    lines.back().visible_end = segment_start;
                    lines.push_back(Line(segment_start));
                }

                wrapBigWord(segment_start, prev_it, max_width);
            } else {
                if (lines.back().width + word_width > max_width) {
                    lines.back().visible_end = segment_start;
                    lines.push_back(Line(segment_start));
                }

                lines.back().width += word_width;
            }

            if (!cont)
                lines.back().visible_end = prev_it;

            if (unicodeIsLineBreak(c)) {
                lines.back().visible_end = prev_it;
                lines.push_back(Line(str_it));
                state = NEWLINE;
            } else {
                state = SPACE;
                segment_start = prev_it;
            }

            break;
        }

        case SPACE: {
            if (unicodeIsSpace(c))
                break;

            float spaces_width = ImGui::CalcTextSize(&*segment_start, &*prev_it).x;
            bool cut_spaces = lines.back().width + spaces_width > max_width;

            if (cut_spaces)
                cutSpaces(segment_start, prev_it, max_width);
            else
                lines.back().width += spaces_width;

            if (cut_spaces || unicodeIsLineBreak(c)) {
                string::iterator line_start;
                if (unicodeIsLineBreak(c))
                    line_start = str_it;
                else
                    line_start = prev_it;

                lines.push_back(Line(line_start));
            }

            if (!cut_spaces && unicodeIsLineBreak(c))
                lines.back().visible_end = prev_it;

            if (unicodeIsLineBreak(c)) {
                state = NEWLINE;
            } else {
                state = WORD;
                segment_start = prev_it;
            }

            break;
        }

        case NEWLINE: {
            if (unicodeIsLineBreak(c)) {
                lines.back().visible_end = prev_it;
                lines.push_back(Line(str_it));
                break;
            }

            segment_start = prev_it;

            if (unicodeIsSpace(c))
                state = SPACE;
            else
                state = WORD;

            break;
        }
        }
    }
}

void WrappedText::prepareDecorators(StrConstIt start_it) {
    size_t i = 0;

    while (i < dec_positions.size() && dec_positions[i].str_it < start_it)
        ++i;

    dec_positions.resize(i);

    for (auto &dec : decorators)
        dec->pushPositions(start_it, dec_positions);

    std::sort(dec_positions.begin(), dec_positions.end(), [](const auto &a, const auto &b) {
        return a.str_it < b.str_it;
    });
}

void WrappedText::draw(float max_width) {
    ImVec2 screen_pos = ImGui::GetCursorPos();
    float original_x = screen_pos.x;

    if (do_calc_lines || lines_calc_for_width != max_width) {
        calcLines(max_width);
        do_calc_lines = false;
    }

    for (auto dec : decorators)
        dec->onDrawStarted();

    auto dp_it = dec_positions.begin();

    for (auto line_it = lines.begin(); line_it != lines.end(); ++line_it) {
        screen_pos.x = original_x;

        StrConstIt draw_start = line_it->start;

        while (draw_start != line_it->visible_end) {
            bool draw_until_dp =
                dp_it != dec_positions.end() && dp_it->str_it < line_it->visible_end;

            StrConstIt draw_end;

            if (draw_until_dp)
                draw_end = dp_it->str_it;
            else
                draw_end = line_it->visible_end;

            ImGui::SetCursorPos(screen_pos);
            ImGui::TextUnformatted(&*draw_start, &*draw_end);

            if (draw_until_dp) {
                screen_pos.x += ImGui::CalcTextSize(&*draw_start, &*draw_end).x;

                dp_it->decorator->onReachedPosition(*dp_it, line_it, screen_pos);
                ++dp_it;
            }

            draw_start = draw_end;
        }

        StrConstIt line_real_end;
        if (line_it + 1 != lines.end())
            line_real_end = (line_it + 1)->start;
        else
            line_real_end = str.end();

        while (dp_it != dec_positions.end() && dp_it->str_it < line_real_end &&
               dp_it->str_it >= line_it->visible_end) {
            dp_it->decorator->onReachedPosition(
                *dp_it,
                line_it,
                ImVec2(original_x + line_it->width, screen_pos.y)
            );
            ++dp_it;
        }

        screen_pos.y += ImGui::GetFontSize();
    }

    for (auto dec : decorators)
        dec->onDrawEnded();
}

void RandomColorTextDecorator::onReachedPosition(
    WrappedText::DecoratorPosition &,
    WrappedText::LineConstIt,
    ImVec2
) {

    if (pop_color)
        ImGui::PopStyleColor();

    uint8_t r = rand() % 256, g = rand() % 256, b = rand() % 256;
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(r, g, b, 255));

    pop_color = true;
}

RandomColorTextDecorator::RandomColorTextDecorator(const WrappedText &_text)
    : WrappedText::Decorator(_text) {}

void RandomColorTextDecorator::onDrawStarted() {
    srand(0);
}

void RandomColorTextDecorator::onDrawEnded() {
    if (pop_color) {
        ImGui::PopStyleColor();
        pop_color = false;
    }
}

void RandomColorTextDecorator::onTextUpdated(WrappedText::StrConstIt) {}

void RandomColorTextDecorator::pushPositions(
    WrappedText::StrConstIt,
    vector<WrappedText::DecoratorPosition> &positions
) {
    auto text_it = text.getString().begin();
    auto text_end = text.getString().end();
    while (text_it != text_end) {
        positions.push_back({text_it, this});
        utf8::next(text_it, text_end);
    }
}

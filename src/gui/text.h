#pragma once

#include "imgui.h"
#include <cmath>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

namespace chat_gui {

class CannotFitGlyphException : public std::exception {
    std::string msg;

  public:
    CannotFitGlyphException(uint32_t unicode, float max_width);
    const char *what() const noexcept;
};

class WrappedText {
  public:
    using StrConstIt = std::string::const_iterator;

    struct Line {
        StrConstIt start;
        float width = 0;
        // Points to the first character that is not rendered on this line.
        // visible_end != text.end() or visible_end != next_line.start only if this line ends with
        // whitespace that exceeds `max_width`.
        StrConstIt visible_end;

        Line(StrConstIt start);
    };

    using LineConstIt = std::vector<Line>::const_iterator;

    class Decorator;

    struct DecoratorPosition {
        StrConstIt str_it;
        Decorator *decorator;
    };

    class Decorator {
      protected:
        const WrappedText &text;

      public:
        Decorator(const WrappedText &text);

        virtual void onDrawStarted() = 0;

        virtual void onDrawEnded() = 0;

        virtual void onTextUpdated(StrConstIt from) = 0;

        virtual void
        onReachedPosition(DecoratorPosition &pos, LineConstIt line_it, ImVec2 screen_pos) = 0;

        virtual void
        pushPositions(StrConstIt start_it, std::vector<DecoratorPosition> &positions) = 0;
    };

    void collectDecoratorPositions();

  private:
    std::string str;
    std::vector<Line> lines;
    std::vector<Decorator *> decorators;
    std::vector<DecoratorPosition> dec_positions;
    // `lines` are calculated for this max width value.
    float lines_calc_for_width = std::numeric_limits<float>::infinity();
    // if true, then `calcLines()` must be called at the begining of `draw()`.
    bool do_calc_lines = true;

    // Used by `calcLines()`.
    //
    // Makes wraps in the word (adds new lines).
    // Call only if word width exceeds the `max_width`.
    // The current line must be empty.
    void wrapBigWord(StrConstIt word_start, StrConstIt word_end, float max_width);

    // Used by `calcLines()`.
    //
    // Calculate what space characters can be rendered on the current line.
    // Call only if sum of space characters width and line width exceeds the `max_width`.
    // Updates only the current line data.
    void cutSpaces(StrConstIt spaces_start, StrConstIt spaces_end, float max_width);

    void calcLines(float max_width);

    void prepareDecorators(StrConstIt start_it);

  public:
    WrappedText(std::string str);

    inline void addDecorator(Decorator *d) {
        decorators.push_back(d);
    }

    inline void prepareDecorators() {
        prepareDecorators(str.begin());
    }

    inline const std::string &getString() const {
        return str;
    }

    inline const std::vector<Line> &getLines() const {
        return lines;
    }

    inline float getHeight() const {
        return lines.size() * ImGui::GetFontSize();
    }

    void replace(size_t start_idx, size_t end_idx, std::string &new_str);

    void draw(float max_width);
};

// Example

class RandomColorTextDecorator : public WrappedText::Decorator {
    bool pop_color = false;

  public:
    RandomColorTextDecorator(const WrappedText &text);

    void onDrawStarted();
    void onDrawEnded();
    void onTextUpdated(const WrappedText::StrConstIt from);
    void onReachedPosition(
        WrappedText::DecoratorPosition &pos,
        WrappedText::LineConstIt line_it,
        ImVec2 screen_pos
    );
    void pushPositions(
        WrappedText::StrConstIt start_it,
        std::vector<WrappedText::DecoratorPosition> &callbacks
    );
};

} // namespace chat_gui

#pragma once

#include "defs.hpp"
#include "log.hpp"
#include <boost/format/format_fwd.hpp>
#include <boost/range/irange.hpp>
#include <cassert>
#include <cmath>
#include <speex/speex_preprocess.h>
#include <speex/speexdsp_config_types.h>

namespace chat {
namespace aud {

class SpeexPreprocessor {
  public:
    SpeexPreprocessor();
    ~SpeexPreprocessor();
    bool process(MonoFrame &frame);
    void control(int request, void *ptr);

  private:
    SpeexPreprocessState *state;
};

} // namespace aud
} // namespace chat

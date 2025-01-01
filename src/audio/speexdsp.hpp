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
    
    ~SpeexPreprocessor() {
        speex_preprocess_state_destroy(state);
    }

    bool process(const MonoFrame &in, MonoFrame &out);

    void control(int request, void *ptr) {
        if (speex_preprocess_ctl(state, request, ptr) == -1) {
            CHAT_LOGE(boost::format("Unknown Speex preprocess request: %1%") % request);
        }
    }

  private:
    SpeexPreprocessState *state;
};

} // namespace aud
} // namespace chat

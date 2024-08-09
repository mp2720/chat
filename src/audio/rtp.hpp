/******************************************************************************
 *   Copyright (c) 2013-2015 thundernet development group, inc.
 *   http://thundernet.com
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a
 *   copy of this software and associated documentation files (the "Software"),
 *   to deal in the Software without restriction, including without limitation
 *   the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *   and/or sell copies of the Software, and to permit persons to whom the
 *   Software is furnished to do so, subject to the following conditions:
 *
 *   1. The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *   2. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *   -----
 *
 ******************************************************************************/
#pragma once

#include "audio/audio.hpp"
#include "audio/codec.hpp"
#include <boost/asio.hpp>
#include <boost/asio/ip/udp.hpp>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

// from RFC 3550, fixed headers for RTP.  This should be all
//  that we need to reference from our perspective
/*
5.1 RTP Fixed Header Fields


      The RTP header has the following format:

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           timestamp                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           synchronization source (SSRC) identifier            |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |            contributing source (CSRC) identifiers             |
   |                             ....                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   If the X bit in the RTP header is one, a variable-length header
   extension is appended to the RTP header, following the CSRC list if
   present. The header extension contains a 16-bit length field that
   counts the number of 32-bit words in the extension, excluding the
   four-octet extension header (therefore zero is a valid length). Only
   a single extension may be appended to the RTP data header.

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      defined by profile       |           length              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        header extension                       |
   |                             ....                              |

*/

#define RTP_VERSION 2
#define RTP_HEADER_LENGTH 12

// RTP flags masks -----
#define RTP_FLAGS_VERSION 0xC000      // 1100 0000 0000 0000
#define RTP_FLAGS_PADDING 0x2000      // 0010 0000 0000 0000
#define RTP_FLAGS_EXTENSION 0x1000    // 0001 0000 0000 0000
#define RTP_FLAGS_CSRC_COUNT 0x0F00   // 0000 1111 0000 0000
#define RTP_FLAGS_MARKER_BIT 0x0080   // 0000 0000 1000 0000
#define RTP_FLAGS_PAYLOAD_TYPE 0x007F // 0000 0000 0111 1111

// RTP payload types -----
#define RTP_PAYLOAD_G711U 0x00
#define RTP_PAYLOAD_GSM 0x03
#define RTP_PAYLOAD_L16 0x0b
#define RTP_PAYLOAD_G729A 0x12
#define RTP_PAYLOAD_SPEEX 0x61
#define RTP_PAYLOAD_DYNAMIC 0x79

namespace aud {

#pragma pack(push, 1)
typedef struct {
    uint16_t flags;     // 2 bytes
    uint16_t sequence;  // 2 bytes
    uint32_t timestamp; // 4 bytes
    uint32_t ssrc;      // 4 bytes
} RTPHeader, *PRTPHeader;

typedef struct {
    uint16_t profile_specific;
    uint16_t ext_length;  // length of ext_data array
    uint32_t ext_data[1]; // array length defined above
} RTPHeaderExt;
#pragma pack(pop)

class RTPPacket {

  public:
    uint8_t *pData;
    uint16_t nLen;
    uint16_t payload_ms;
    uint8_t payload_type;
    uint16_t payload_bytes;
    bool use_redundant_payload;

    RTPPacket(uint8_t *pIn, short nInLen) : pData(NULL), nLen(nInLen) {
        payload_ms = 0;
        payload_type = RTP_PAYLOAD_G711U;
        payload_bytes = 0;
        use_redundant_payload = false;
        if (pIn && nLen) {
            pData = new uint8_t[nLen];
            if (pData) {
                memcpy(pData, pIn, nInLen);
            };
        }
    }
    ~RTPPacket() {
        if (pData)
            delete[] pData;
    };
};

typedef std::shared_ptr<RTPPacket> rawrtp_ptr;

using namespace boost::asio;

class RtpOutput : public Output {
  public:
    RtpOutput(std::shared_ptr<ip::udp::socket> conn, ip::udp::endpoint addr, EncoderPreset ep, int channels);
    virtual void stop() override;
    virtual void start() override;
    virtual int channels() const override;
    virtual void write(Frame &frame) override;

  private:
    std::shared_ptr<ip::udp::socket> conn;
    
    OpusEnc enc;
    int chans;
    uint16_t sequence = 0;
    std::vector<uint8_t> encBuf;
    std::vector<uint8_t> rtpBuf;
    ip::udp::endpoint addr;
};

} // namespace aud

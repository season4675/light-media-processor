/*
 * Copyright 2025 Alibaba Group Holding Limited
 * Copyright 2025 shichen.fsc <shichen.fsc@alibaba-inc.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define TAG "OGGOPUS_DECODER"

#include "oggopus_decoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "media_code.h"
#include "oggopus_constants.h"
#include "oggopus_header.h"
#include "spdlog/spdlog.h"

#define DEFAULT_CHANNELS 1

namespace funaudio {

OggOpusDataDecoder::OggOpusDataDecoder()
    : ogg_opus_para_(NULL),
      sample_rate_(16000),
      channels_(1),
      debug_(false),
      output_buffer_(NULL),
      output_buf_samples_(0) {}
OggOpusDataDecoder::~OggOpusDataDecoder() {}

int OggOpusDataDecoder::OggopusDecoderCreate(int sample_rate, int channels) {
  channels_ = channels;
  sample_rate_ = sample_rate;
  if (ogg_opus_para_) {
    delete ogg_opus_para_;
    ogg_opus_para_ = NULL;
  }
  ogg_opus_para_ = new OggOpusDataDecoderPara();
  if (NULL == ogg_opus_para_) {
    return -(kOggOpusCreateFailed);
  }

  /* reset opus header. decoder no need opus_header. */
  ogg_opus_para_->header.version = 1;  // RFC 7845
  ogg_opus_para_->header.channels = channels;
  ogg_opus_para_->header.input_sample_rate = sample_rate;
  ogg_opus_para_->header.gain = 0;
  // ogg_opus_para_->header.nb_streams = channels;
  // ogg_opus_para_->header.nb_coupled =
  //     channels - ogg_opus_para_->header.nb_streams;
  // ogg_opus_para_->header.gain = 0;
  // ogg_opus_para_->header.channel_mapping = channels - 1;
  // for (int i = 0; i < channels; i++) {
  //   ogg_opus_para_->header.stream_map[i] = i;
  // }
  if (channels <= 2) {
    // 单声道或立体声：使用 channel_mapping=0
    ogg_opus_para_->header.channel_mapping = 0;
    ogg_opus_para_->header.nb_streams = 1;
    ogg_opus_para_->header.nb_coupled = channels == 1 ? 0 : 1;
    memset(ogg_opus_para_->header.stream_map, 0,
           sizeof(ogg_opus_para_->header.stream_map));
    // nb_streams/nb_coupled/stream_map 在 mapping=0 时被忽略
  } else {
    // 多通道：使用 channel_mapping=1（Vorbis 映射）
    ogg_opus_para_->header.channel_mapping = 1;

    // 根据通道数设置流结构（参考 Vorbis 标准）
    if (channels == 3) {
      // 3.0: L, R, C
      ogg_opus_para_->header.nb_streams = 2;
      ogg_opus_para_->header.nb_coupled = 1;     // L+R 耦合
      ogg_opus_para_->header.stream_map[0] = 0;  // L
      ogg_opus_para_->header.stream_map[1] = 1;  // R
      ogg_opus_para_->header.stream_map[2] = 2;  // C
    } else if (channels == 4) {
      // Quad: FL, FR, RL, RR
      ogg_opus_para_->header.nb_streams = 2;
      ogg_opus_para_->header.nb_coupled = 2;     // 两对立体声
      ogg_opus_para_->header.stream_map[0] = 0;  // FL
      ogg_opus_para_->header.stream_map[1] = 1;  // FR
      ogg_opus_para_->header.stream_map[2] = 2;  // RL
      ogg_opus_para_->header.stream_map[3] = 3;  // RR
    } else if (channels == 6) {
      // 5.1: FL, FR, FC, LFE, RL, RR
      ogg_opus_para_->header.nb_streams = 4;
      ogg_opus_para_->header.nb_coupled = 2;     // FL/FR + RL/RR
      ogg_opus_para_->header.stream_map[0] = 0;  // FL
      ogg_opus_para_->header.stream_map[1] = 1;  // FR
      ogg_opus_para_->header.stream_map[2] = 2;  // FC
      ogg_opus_para_->header.stream_map[3] = 3;  // LFE
      ogg_opus_para_->header.stream_map[4] = 4;  // RL
      ogg_opus_para_->header.stream_map[5] = 5;  // RR
    } else {
      // 非标准通道数：保守处理（全部单声道）
      ogg_opus_para_->header.nb_streams = channels;
      ogg_opus_para_->header.nb_coupled = 0;
      for (int i = 0; i < channels; i++) {
        ogg_opus_para_->header.stream_map[i] = i;
      }
    }
  }

  ogg_sync_init(&ogg_opus_para_->sync);
  ogg_opus_para_->has_stream = false;
  ogg_opus_para_->has_page = false;

  return OpusDecoderTryCreate(sample_rate, channels);
}

int OggOpusDataDecoder::OggopusDestroy() {
  if (ogg_opus_para_) {
    if (ogg_opus_para_->opus_multistream_decoder) {
      opus_multistream_decoder_destroy(
          ogg_opus_para_->opus_multistream_decoder);
      ogg_opus_para_->opus_multistream_decoder = NULL;
    }

    if (ogg_opus_para_->has_stream) {
      // ogg_stream_destroy: always return 0
      ogg_stream_clear(&ogg_opus_para_->os);
    }
    // ogg_sync_destroy: always return 0
    ogg_sync_clear(&ogg_opus_para_->sync);

    output_buf_samples_ = 0;
    if (output_buffer_) {
      delete[] output_buffer_;
      output_buffer_ = NULL;
    }

    delete ogg_opus_para_;
    ogg_opus_para_ = NULL;
  }

  return kSuccess;
}

int OggOpusDataDecoder::OggopusDecode(const uint8_t *frameBuff,
                                      const int frameLen, uint8_t *outputBuffer,
                                      int outputBufferBytes) {
  if (!frameBuff || frameLen <= 0 || !outputBuffer) {
    SPDLOG_ERROR("invalid params");
    return 0;
  }

  int decoded_size = 0;
  int ret = OggReaderPutData((const char *)frameBuff, frameLen);
  if (kSuccess != ret) {
    SPDLOG_ERROR("Put data failed: ret={}!", ret);
  }
  ret = ReadPage(outputBuffer, &decoded_size, outputBufferBytes);
  if (debug_) {
    SPDLOG_INFO("Read page ret={}, decoded_size={}!", ret, decoded_size);
  }
  if (ret == kSuccess) {
    return decoded_size;
  }
  return ret;
}

int OggOpusDataDecoder::OpusDecoderTryCreate(int sample_rate, int channels) {
  if (channels <= 0) {
    channels = DEFAULT_CHANNELS;
  }

  bool recreate = false;
  if (channels_ != channels || sample_rate_ != sample_rate ||
      ogg_opus_para_->opus_multistream_decoder == NULL) {
    recreate = true;
    if (ogg_opus_para_->opus_multistream_decoder) {
      opus_multistream_decoder_destroy(
          ogg_opus_para_->opus_multistream_decoder);
      ogg_opus_para_->opus_multistream_decoder = NULL;
    }
  }

  sample_rate_ = sample_rate;
  channels_ = channels;

  if (recreate) {
    int tmpCode = OPUS_OK;
    int stream_cnt = channels_;
    int coupled_streams_cnt = channels_ - stream_cnt;
    std::vector<unsigned char> mapping(channels);
    for (int i = 0; i < channels_; ++i) {
      mapping[i] = i;
    }
    ogg_opus_para_->opus_multistream_decoder =
        /* 分配和初始化多流解码器状态 */
        opus_multistream_decoder_create(
            sample_rate_,        /* 8000, 16000 */
            channels_,           /* channel count */
            stream_cnt,          /* stream count */
            coupled_streams_cnt, /* coupled stream count */
            mapping.data(), &tmpCode);
    if (tmpCode != OPUS_OK) {
      SPDLOG_ERROR("error cannot create decoder: {}", opus_strerror(tmpCode));
      return -(kOpusDecoderCreateFailed);
    } else {
      if (output_buffer_) {
        delete[] output_buffer_;
        output_buffer_ = NULL;
      }
      output_buf_samples_ = sample_rate_ * 120 / 1000;  // 120ms
      output_buffer_ = new opus_int16[output_buf_samples_ * channels_];

      SPDLOG_DEBUG(
          "opus_multistream_decoder_create success. sample_rate:{} "
          "channels:{} and output_buf_samples:{}",
          sample_rate_, channels_, output_buf_samples_);
    }
  }
  return kSuccess;
}

void OggOpusDataDecoder::SetDebugMode(bool enable) {
  debug_ = enable;
  if (ogg_opus_para_) {
    ogg_opus_para_->debug = debug_;
  }
}

int OggOpusDataDecoder::OggReaderPutData(const char *data, int data_len) {
  char *buffer = ogg_sync_buffer(&ogg_opus_para_->sync, data_len);
  if (!buffer) {
    return -(kOggAllocateMemoryFailed);
  }

  memcpy(buffer, data, static_cast<size_t>(data_len));
  // ogg_sync_wrote: 0 = success; -1 = buffer overflow
  if (0 != ogg_sync_wrote(&ogg_opus_para_->sync, data_len) < 0) {
    return -(kOggBufferOverflow);
  }

  return kSuccess;
}

int OggOpusDataDecoder::OggReaderGetPage(int *serialno, int *granule) {
  // ogg_sync_pageout: -1 = data skipped;
  //                    0 = data consumed but not enough for one page;
  //                    1 = page completed
  int ret = ogg_sync_pageout(&ogg_opus_para_->sync, &ogg_opus_para_->og);
  if (1 == ret) {
    int sn = ogg_page_serialno(&ogg_opus_para_->og);
    if (!ogg_opus_para_->has_stream) {
      // ogg_stream_init: 0 = success; -1 = failed
      if (0 != ogg_stream_init(&ogg_opus_para_->os, sn)) {
        return -(kOggInitFailed);
      }
      ogg_opus_para_->has_stream = true;
    } else if (sn != ogg_opus_para_->os.serialno) {
      ogg_stream_reset_serialno(&ogg_opus_para_->os, sn);
    }
    // ogg_stream_pagein: 0 = success; -1 = failed
    if (0 != ogg_stream_pagein(&ogg_opus_para_->os, &ogg_opus_para_->og)) {
      return -(kOggStreamPageinFailed);
    }
    ogg_opus_para_->has_page = true;
    *serialno = sn;
    *granule = (int)ogg_page_granulepos(&ogg_opus_para_->og);
    return kSuccess;
  } else {
    if (debug_) {
      SPDLOG_WARN("ogg_sync_pageout failed, ret={}.", ret);
    }
    return kOggNoAvailablePage;
  }
}

int OggOpusDataDecoder::OggReaderGetPacket(char **data, int *data_len,
                                           int *b_o_s, int *e_o_s) {
  if (!ogg_opus_para_->has_page) {
    return -(kOggNoAvailablePage);
  }

  // ogg_stream_packetout: -1 = out of sync and there is a gap in the data;
  //                        0 = data consumed but not enough for one packet;
  //                        1 = packet completed
  int ret = ogg_stream_packetout(&ogg_opus_para_->os, &ogg_opus_para_->op);
  if (1 == ret) {
    *data = (char *)ogg_opus_para_->op.packet;
    *data_len = (int)ogg_opus_para_->op.bytes;
    *b_o_s = (int)ogg_opus_para_->op.b_o_s;
    *e_o_s = (int)ogg_opus_para_->op.e_o_s;
    return kSuccess;
  } else {
    if (ret == 0) {
      SPDLOG_DEBUG("ogg_stream_packetout failed, ret={}.", ret);
    } else {
      SPDLOG_WARN("ogg_stream_packetout failed, ret={}.", ret);
    }
    ogg_opus_para_->has_page = false;
    return kOggNoAvailablePacket;
  }
}

int OggOpusDataDecoder::OggReaderReset() {
  if (ogg_opus_para_->has_stream) {
    // ogg_stream_clear: always return 0
    ogg_stream_reset(&ogg_opus_para_->os);
    ogg_opus_para_->has_stream = false;
    ogg_opus_para_->has_page = false;
  }
  // ogg_sync_clear: always return 0
  ogg_sync_reset(&ogg_opus_para_->sync);

  ogg_opus_para_->has_opus_stream = false;
  ogg_opus_para_->opus_serialno = 0;
  ogg_opus_para_->opus_granule = 0;
  ogg_opus_para_->packet_count = 0;

  return kSuccess;
}

bool OggOpusDataDecoder::OpusParseHeader(unsigned char *data, int data_len) {
  int packet_size = OpusHeaderToPacket(&ogg_opus_para_->header, data, data_len);
  if (debug_) {
    SPDLOG_DEBUG(
        "Parse header finished: version={:x}, channels={:u}, preskip={:u},"
        " input_sample_rate={}, gain={:u}, channel_mapping={:u}, "
        "nb_streams={:u}, "
        "nb_coupled={:u}",
        ogg_opus_para_->header.version, ogg_opus_para_->header.channels,
        ogg_opus_para_->header.preskip,
        ogg_opus_para_->header.input_sample_rate, ogg_opus_para_->header.gain,
        ogg_opus_para_->header.channel_mapping,
        ogg_opus_para_->header.nb_streams, ogg_opus_para_->header.nb_coupled);
  }
  if (packet_size <= 0) {
    SPDLOG_ERROR("Parse header failed!");
    return false;
  }
  sample_rate_ = ogg_opus_para_->header.input_sample_rate;
  return true;
}

int OggOpusDataDecoder::ReadPacket(int serialno, uint8_t *out,
                                   int *decoded_size, int *out_offset,
                                   int outputBufferBytes) {
  int ret = kSuccess;
  while (true) {
    if (debug_) {
      SPDLOG_TRACE("Getting packet ...");
    }
    char *data = NULL;
    int data_len, bos, eos;
    ret = OggReaderGetPacket(&data, &data_len, &bos, &eos);
    if (kSuccess == ret) {
      if (debug_) {
        SPDLOG_DEBUG("Got packet: len={}, bos={}, eos={}", data_len, bos, eos);
      }
      if (bos && data_len > 8 && !memcmp(data, "OpusHead", 8)) {
        ogg_opus_para_->has_opus_stream = true;
        ogg_opus_para_->opus_serialno = serialno;
      }

      if (!ogg_opus_para_->has_opus_stream ||
          ogg_opus_para_->opus_serialno != serialno) {
        SPDLOG_ERROR("Skipped page.");
        return kSuccess;
      }
      ++ogg_opus_para_->packet_count;
      if (1 == ogg_opus_para_->packet_count) {
        // Packet #1 is the OPUS header.
        ret = OpusParseHeader((unsigned char *)data, data_len);
        if (ret) {
          if (debug_) {
            SPDLOG_DEBUG("Creating opus decoder: sample_rate={} ...",
                         sample_rate_);
          }
          ret = OpusDecoderTryCreate(sample_rate_, channels_);
          if (kSuccess != ret) {
            SPDLOG_ERROR("Create opus decoder failed: status={}", ret);
            return ret;
          }
        } else {
          return ret;
        }
      } else if (2 == ogg_opus_para_->packet_count) {
        // Packet #2 is a comment packet.
        SPDLOG_DEBUG("Skipped comment packet.");
      } else {
        // Decoding packet.
        ret = DecodePacket(data, data_len, reinterpret_cast<char *>(out),
                           decoded_size, out_offset, outputBufferBytes);
        if (!ret) {
          return ret;
        }
      }
    } else if (kOggNoAvailablePacket == ret) {
      if (debug_) {
        SPDLOG_DEBUG("All packets fetched");
      }
      return kSuccess;
    } else {
      SPDLOG_ERROR("Get packet failed: status={}!", ret);
      return -(ret);
    }
  }  // while
  return ret;
}

int OggOpusDataDecoder::DecodePacket(char *data, int data_len, char *out,
                                     int *out_len, int *out_offset,
                                     int outputBufferBytes) {
  if (debug_) {
    SPDLOG_TRACE(" ==> Decoding packet ...");
  }
  int ret = OpusDecoderDecode(data, data_len, out, out_len, out_offset,
                              outputBufferBytes);
  if (kSuccess != ret) {
    SPDLOG_ERROR("Decode packet failed: ret={}!", ret);
    return ret;
  } else {
    if (debug_) {
      SPDLOG_TRACE(" <== Decoded {}bytes from {}bytes opus data.", *out_len,
                   data_len);
    }
  }

  return ret;
}

int OggOpusDataDecoder::OpusDecoderDecode(const char *in_data, int in_len,
                                          char *out, int *out_len,
                                          int *out_offset,
                                          int outputBufferBytes) {
  if (!in_data || in_len <= 0 || !out) {
    return -(kOpusInvalidParameter);
  }

  if (debug_) {
    SPDLOG_TRACE(
        "  Try to decoding {}bytes data, output_buf is {} samples, now "
        "decoded_bytes is {}, out offset is {} and output_buf limits {}bytes.",
        in_len, output_buf_samples_, *out_len, *out_offset, outputBufferBytes);
  }

  /*
   * Returns
   *  Number of samples decoded on success or a negative error code (see Error
   * codes) on failure.
   */
  int decoded_size = opus_multistream_decode(
      ogg_opus_para_->opus_multistream_decoder, (const unsigned char *)in_data,
      in_len, output_buffer_, output_buf_samples_, 0);
  if (decoded_size < 0) {
    // Map the opus error code to positive integers.
    return decoded_size;
  }

  int decoded_bytes = decoded_size * 2 * channels_;
  int offset = *out_offset;
  if (debug_) {
    SPDLOG_TRACE("  Decoded {}bytes.", decoded_bytes);
  }
  if (outputBufferBytes > 0 && offset + decoded_bytes > outputBufferBytes) {
    int cur_decoded_bytes = outputBufferBytes - offset;
    if (cur_decoded_bytes > 0) {
      SPDLOG_WARN(
          "  Offset {} + decoded_bytes {} > limited buffer {}. Now modify "
          "decoded bytes to {}.",
          offset, decoded_bytes, outputBufferBytes, cur_decoded_bytes);
      decoded_bytes = cur_decoded_bytes;
      decoded_size = cur_decoded_bytes / 2;
    } else {
      SPDLOG_WARN(
          "  Out buffer is overflow!! Offset is {} and limited buffer {}.",
          offset, outputBufferBytes);
      return kSuccess;
    }
  }
  char *cur_out_ptr = out + offset;
  *out_len += decoded_bytes;
  *out_offset += decoded_bytes;

  // short array to bytes array
  for (int i = 0; i < decoded_size * channels_; i++) {
    cur_out_ptr[i * 2] = output_buffer_[i] & 0xFF;
    cur_out_ptr[i * 2 + 1] = (output_buffer_[i] >> 8) & 0xFF;
  }
  return kSuccess;
}

int OggOpusDataDecoder::ReadPage(uint8_t *out, int *decoded_size,
                                 int outputBufferBytes) {
  int ret = kSuccess;
  int out_offset = 0;
  while (true) {
    int serialno = 0, granule = 0;
    ret = OggReaderGetPage(&serialno, &granule);
    if (kSuccess == ret) {
      if (debug_) {
        SPDLOG_DEBUG("Got page done, serialno={}, granule={}", serialno,
                     granule);
      }
      ogg_opus_para_->opus_granule = granule;
      ret = ReadPacket(serialno, out, decoded_size, &out_offset,
                       outputBufferBytes);
      if (ret != kSuccess) {
        return ret;
      }
    } else if (kOggNoAvailablePage == ret) {
      if (debug_) {
        SPDLOG_ERROR("All pages fetched");
      }
      return kSuccess;
    } else {
      SPDLOG_ERROR("Get page failed: status={}!", ret);
      return -(ret);
    }
  }  // while
  return ret;
}

}  // namespace funaudio

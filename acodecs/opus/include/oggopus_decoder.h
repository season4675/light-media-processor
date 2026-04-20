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

#pragma once

#include "oggopus_data_struct.h"

namespace funaudio {

class OggOpusDataDecoderPara {
 public:
  OggOpusDataDecoderPara()
      : opus_multistream_decoder(NULL),
        has_opus_stream(false),
        opus_serialno(0),
        opus_granule(0),
        packet_count(0),
        debug(false) {}
  ~OggOpusDataDecoderPara() {}

 public:
  OpusMSDecoder* opus_multistream_decoder;
  /* I/O */
  ogg_sync_state sync;
  ogg_stream_state os;  // 代表当前流
  ogg_page og;          // 编码时page的信息在此输出
  ogg_packet op;        // 编码时数据输入的结构包
  OpusHeader header;
  bool has_stream;
  bool has_page;
  bool debug;
  /* Settings */
  bool has_opus_stream;
  int opus_serialno;
  int opus_granule;
  int packet_count;
};

class OggOpusDataDecoder {
 public:
  OggOpusDataDecoder();
  ~OggOpusDataDecoder();

  /**
   * @brief 初始化OggOpus解码器
   * @param sample_rate 采样率, 目前已经验证8K,16K,48K
   * @param channels 采样率, 目前支持单通道,双通道
   * @return
   */
  int OggopusDecoderCreate(int sample_rate = 16000, int channels = 1);
  int OggopusDecode(const uint8_t* frameBuff, const int frameLen,
                    uint8_t* outputBuffer, int outputBufferBytes = 0);
  int OggopusDestroy();

  void SetDebugMode(bool enable);

 private:
  int OpusDecoderTryCreate(int sample_rate, int channels);
  int OggReaderPutData(const char* data, int data_len);
  int OggReaderGetPage(int* serialno, int* granule);
  int OggReaderGetPacket(char** data, int* data_len, int* b_o_s, int* e_o_s);
  int OggReaderReset();
  bool OpusParseHeader(unsigned char* data, int data_len);
  int ReadPacket(int serialno, uint8_t* out, int* decoded_size, int* out_offset,
                 int outputBufferBytes = 0);
  int DecodePacket(char* data, int data_len, char* out, int* out_len,
                   int* out_offset, int outputBufferBytes = 0);
  int OpusDecoderDecode(const char* in_data, int in_len, char* out,
                        int* out_len, int* out_offset,
                        int outputBufferBytes = 0);
  int ReadPage(uint8_t* out, int* decoded_size, int outputBufferBytes = 0);

  OggOpusDataDecoderPara* ogg_opus_para_;
  int channels_;
  int sample_rate_;
  bool debug_;
  opus_int16* output_buffer_;
  /* The number of samples per channel of available space in pcm. */
  int output_buf_samples_;
};

}  // namespace funaudio

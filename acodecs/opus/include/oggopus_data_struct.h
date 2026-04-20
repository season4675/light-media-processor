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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ogg/ogg.h"
#include "oggopus_header.h"
#include "opus_multistream.h"
#include "opus_types.h"

namespace mproc {

// typedef size_t (*EncodedDataCallback)(const unsigned char *encoded_data,
//                                       int len, void *user_data);
typedef int64_t (*ReadFunc)(void *src, float *buffer, int samples,
                            char **buffers, int *lenth);

struct WavInfo {
  uint16_t channels;
  int16_t sample_bits;
  opus_int64 samplesread;
  int16_t bigendian;
  int16_t unsigned8bit;
  int *channel_permute;
  WavInfo()
      : channels(0),
        sample_bits(0),
        samplesread(0),
        bigendian(0),
        unsigned8bit(0),
        channel_permute(NULL) {}
};

struct Padder {
  ReadFunc read_func;
  void *read_info;
  ogg_int64_t *original_sample_number;
  int channels;
  int lpc_ptr;
  int *extra_samples;
  float *lpc_out;
  Padder()
      : read_func(NULL),
        read_info(NULL),
        original_sample_number(NULL),
        channels(0),
        lpc_ptr(0),
        extra_samples(NULL),
        lpc_out(NULL) {}
};

struct OggEncodeOpt {
  ReadFunc read_func;
  // EncodedDataCallback callback_data_func;
  char *comments;
  void *user_data;
  void *read_info;
  opus_int64 total_samples_per_channel;
  int channels;
  int sample_bits;
  int endianness;
  int ignorelength;
  int skip;
  int extraout;
  int comments_length;
  int copy_comments;
  int copy_pictures;
  OggEncodeOpt()
      : read_func(NULL),
        // callback_data_func(NULL),
        comments(NULL),
        user_data(NULL),
        read_info(NULL),
        total_samples_per_channel(0),
        channels(0),
        sample_bits(0),
        endianness(0),
        ignorelength(0),
        skip(0),
        extraout(0),
        comments_length(0),
        copy_comments(0),
        copy_pictures(0) {}
};

struct OggDecodePageHeader {
  char Oggs[4] = {0};
  unsigned char ver = 0;
  // 这1字节字段中的位标识该页面的特定类型
  // 0x01=continued, 0x02=first page, 0x04=last page
  unsigned char header_type_flag = 0;
  unsigned char granule_position[8] = {0};
  // 包含唯一序列号的4字节字段，通过该唯一序列号来识别逻辑比特流
  unsigned char stream_serial_num[4] = {0};
  // 包含页面序列号的4字节字段，使得解码器可以识别页面丢失。该序列号在每个逻辑比特流上分别增加
  unsigned char page_sequence_number[4] = {0};
  // 包含页面的32位CRC校验和的4字节字段
  unsigned char CRC_checksum[4] = {0};
  // 1字节，给出分段表(segment table)中编码的分段条目的数量
  unsigned char seg_num = 0;

  // unsigned char segment_table[];
  // unsigned char segment_table[256] = {0};

  const unsigned char* segment_table() const {
      return reinterpret_cast<const unsigned char*>(this) + fixed_header_size();
  }

  static constexpr size_t fixed_header_size() {
      return sizeof(Oggs) + sizeof(ver) + sizeof(header_type_flag) +
              sizeof(granule_position) + sizeof(stream_serial_num) +
              sizeof(page_sequence_number) + sizeof(CRC_checksum) + sizeof(seg_num);
  }
};

struct AudioFunctions {
  void (*open_func)(OggEncodeOpt *opt);
  void (*close_func)(void *);
};

#define readint(buf, base)                                                     \
  (((buf[base + 3] << 24) & 0xff000000) | ((buf[base + 2] << 16) & 0xff0000) | \
   ((buf[base + 1] << 8) & 0xff00) | (buf[base] & 0xff))

#define writeint(buf, base, val)          \
  do {                                    \
    buf[base + 3] = ((val) >> 24) & 0xff; \
    buf[base + 2] = ((val) >> 16) & 0xff; \
    buf[base + 1] = ((val) >> 8) & 0xff;  \
    buf[base] = (val)&0xff;               \
  } while (0)

}  // namespace mproc

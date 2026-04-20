
#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include "opus/config.h"
#include "include/opus_types.h"
#include "include/opus_multistream.h"
#include "oggopus_data_struct.h"

namespace mproc {

/*
 * 这是一个用于封装 Opus 音频编码器和 Ogg 容器格式写入逻辑的参数类。它整合了 Opus 编码器的状态、Ogg 流的状态、输入输出缓冲区以及编码配置参数。
 */
class OggOpusEncoderPara {
 public:
  OggOpusEncoderPara()
      : opus_ms_encoder_(nullptr),
        opus_version_(nullptr),
        packet_(nullptr),
        input_pcm16_(nullptr),
        input_pcm24(nullptr),
        input_pcm32_(nullptr),
        input_samples_(0),
        audio_functions(nullptr),
        last_granulepos(0),
        enc_granulepos(0),
        original_sample_number(0),
        id(0),
        last_segments(-1),
        nbBytes(0),
        nb_samples(0),
        start_time(0),
        max_frame_bytes_(0),
        complexity(0),
        max_ogg_delay(0),
        serialno(0),
        lookahead(0),
        debug(false) {
    snprintf(ENCODER_string, sizeof(ENCODER_string), "opusenc from %s",
             PACKAGE_STRING);
  }
  ~OggOpusEncoderPara() {}

  int InitComment();
  int AddComment();
  int AddUserComment(std::string comment);
  int WritePage();
  void PrintOggPageHeader(const uint8_t *header, const int header_len);
  unsigned int ToUInt(unsigned char num[4], int len);
  unsigned long long ToULL(unsigned char num[8], int len);

 public:
  OpusMSEncoder *opus_ms_encoder_;
  const int8_t *opus_version_;
  uint8_t *packet_; // 输出编码数据缓冲区
  std::vector<opus_int16> cache_pcm16_;
  opus_int16 *input_pcm16_; // 从缓存得到的待编码 PCM 数据 (opus_int16 格式)
  std::vector<opus_int32> cache_pcm24_;
  opus_int32 *input_pcm24; // 从缓存得到的待编码 PCM 数据 (opus_int32 格式)
  std::vector<float> cache_pcm32_;
  float *input_pcm32_; // 从缓存得到的待编码 PCM 数据 (float 格式)
  size_t input_samples_; // 从缓存得到的待编码样本数
  /* I/O */
  OggEncodeOpt ogg_encode_opt;
  const AudioFunctions *audio_functions;
  ogg_stream_state os;  // 代表当前流
  ogg_page og;          // 编码时page的信息在此输出
  ogg_packet op;        // 编码时数据输入的结构包
  ogg_int64_t last_granulepos;
  ogg_int64_t enc_granulepos;
  ogg_int64_t original_sample_number;
  ogg_int32_t id;
  int last_segments;
  OpusHeader header;
  char ENCODER_string[1024];
  /* Counters */
  opus_int32 nbBytes;
  opus_int32 nb_samples;
  time_t start_time;
  /* Settings */
  int max_frame_bytes_; // 输出缓冲区最大大小
  opus_int32 bitrate;
  int complexity;
  int max_ogg_delay; /*48kHz samples*/
  int serialno;
  opus_int32 lookahead;

  bool debug;
};

}
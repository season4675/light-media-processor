/*
 * Copyright (c) 2026 season(season4675@gmail.com). All rights reserved.
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#include "oggopus_encoder.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <numeric>
#include <vector>

#if __cplusplus >= 202002L
#include <span>
#endif

#include "media_code.h"
#include "media_constants.h"
#include "oggopus_audio_in.h"
#include "oggopus_header.h"
#include "spdlog/spdlog.h"

namespace mproc {

namespace {
// 匿名命名空间用于内部辅助函数

constexpr int kDefaultFrameDurationMs = 20;  // 默认帧时长 20ms
constexpr int kDefaultSampleRate = 48000;    // 默认采样率 48kHz
constexpr int kDefaultBitrate = 64000;       // 默认比特率 64kbps
constexpr int kDefaultComplexity = 10;       // 默认复杂度

// PCM 转换辅助函数：将 PCM16 转换为 float
void Pcm16ToFloat(const int16_t* pcm_in, float* pcm_out, size_t samples) {
  constexpr float scale = 1.0f / 32768.0f;
  for (size_t i = 0; i < samples; ++i) {
    pcm_out[i] = pcm_in[i] * scale;
  }
}

// PCM 转换辅助函数：将 PCM24 转换为 float
void Pcm24ToFloat(const int32_t* pcm_in, float* pcm_out, size_t samples) {
  constexpr float scale = 1.0f / 8388608.0f;  // 2^23
  for (size_t i = 0; i < samples; ++i) {
    pcm_out[i] = pcm_in[i] * scale;
  }
}

// PCM 转换辅助函数：将 PCM32 转换为 float
void Pcm32ToFloat(const int32_t* pcm_in, float* pcm_out, size_t samples) {
  constexpr float scale = 1.0f / 2147483648.0f;  // 2^31
  for (size_t i = 0; i < samples; ++i) {
    pcm_out[i] = pcm_in[i] * scale;
  }
}

// 写入 PCM16 到缓存
size_t WritePcm16ToCache(const int16_t* pcm_data, size_t samples,
                         std::vector<int16_t>& cache) {
  cache.insert(cache.end(), pcm_data, pcm_data + samples);
  return cache.size();
}

// 写入 PCM24 到缓存
size_t WritePcm24ToCache(const int32_t* pcm_data, size_t samples,
                         std::vector<int32_t>& cache) {
  cache.insert(cache.end(), pcm_data, pcm_data + samples);
  return cache.size();
}

// 写入 PCM32(float) 到缓存
size_t WritePcm32ToCache(const float* pcm_data, size_t samples,
                         std::vector<float>& cache) {
  cache.insert(cache.end(), pcm_data, pcm_data + samples);
  return cache.size();
}

// 从缓存读取 PCM16 并转换为 float
size_t ReadPcm16FromCache(std::vector<int16_t>& cache, float* output,
                          size_t samples_needed) {
  const size_t samples_available = cache.size();
  const size_t samples_to_read = std::min(samples_available, samples_needed);

  if (samples_to_read > 0) {
    Pcm16ToFloat(cache.data(), output, samples_to_read);
    // 移除已读取的数据
    cache.erase(cache.begin(), cache.begin() + samples_to_read);
  }

  return samples_to_read;
}

// 从缓存读取 PCM24 并转换为 float
size_t ReadPcm24FromCache(std::vector<int32_t>& cache, float* output,
                          size_t samples_needed) {
  const size_t samples_available = cache.size();
  const size_t samples_to_read = std::min(samples_available, samples_needed);

  if (samples_to_read > 0) {
    Pcm24ToFloat(cache.data(), output, samples_to_read);
    // 移除已读取的数据
    cache.erase(cache.begin(), cache.begin() + samples_to_read);
  }

  return samples_to_read;
}

// 从缓存读取 PCM32(float) 
size_t ReadPcm32FromCache(std::vector<float>& cache, float* output,
                          size_t samples_needed) {
  const size_t samples_available = cache.size();
  const size_t samples_to_read = std::min(samples_available, samples_needed);

  if (samples_to_read > 0) {
    std::memcpy(output, cache.data(), samples_to_read * sizeof(float));
    cache.erase(cache.begin(), cache.begin() + samples_to_read);
  }

  return samples_to_read;
}

}  // 匿名命名空间

OggOpusEncoder::OggOpusEncoder()
    : ogg_opus_para_(nullptr),
      is_first_frame_processed_(false),
      channels_(1),
      sample_rate_(kDefaultSampleRate),
      sample_bits_(16),
      frame_duration_ms_(kDefaultFrameDurationMs),
      frame_sample_num_(sample_rate_ * frame_duration_ms_ / 1000),
      frame_sample_bytes_(frame_sample_num_ * channels_ * sizeof(int16_t)),
      enc_bitrate_(kDefaultBitrate),
      enc_complexity_(kDefaultComplexity),
      enable_vbr_(true),
      enable_constraint_vbr_(false),
      app_type_("AUDIO"),
      user_comment_(""),
      debug_(false),
      encoded_data_buffer_(),
      encoded_callback_(nullptr),
      callback_user_data_(nullptr) {}

OggOpusEncoder::~OggOpusEncoder() { Destroy(); }

void OggOpusEncoder::ResetParameters() {
  ogg_opus_para_->last_granulepos = 0;
  ogg_opus_para_->enc_granulepos = 0;
  ogg_opus_para_->original_sample_number = 0;
  ogg_opus_para_->id = 0;
  ogg_opus_para_->last_segments = 0;
  ogg_opus_para_->nbBytes = 0;
  ogg_opus_para_->nb_samples = 0;
  ogg_opus_para_->max_frame_bytes_ =
      frame_sample_num_ * channels_ * sizeof(float) * 2;  // 预留 2 倍空间
  ogg_opus_para_->bitrate = enc_bitrate_;
  ogg_opus_para_->complexity = enc_complexity_;
  ogg_opus_para_->max_ogg_delay = 200;
  ogg_opus_para_->lookahead = 0;

  static const AudioFunctions audio_funcs = {RawOpen, WavClose};
  ogg_opus_para_->audio_functions = &audio_funcs;

  ogg_opus_para_->ogg_encode_opt.sample_bits = sample_bits_;
  ogg_opus_para_->ogg_encode_opt.endianness = 0;
  ogg_opus_para_->ogg_encode_opt.ignorelength = 0;
  ogg_opus_para_->ogg_encode_opt.copy_comments = 1;
  ogg_opus_para_->ogg_encode_opt.copy_pictures = 1;
  ogg_opus_para_->ogg_encode_opt.channels = channels_;
  ogg_opus_para_->ogg_encode_opt.skip = 0;

  ogg_opus_para_->start_time = time(nullptr);

  // opus_get_version_string() 返回 const char*，需要转换
  const char* version_str = opus_get_version_string();
  ogg_opus_para_->opus_version_ = reinterpret_cast<const int8_t*>(version_str);

  SPDLOG_DEBUG("opus_version: {}", std::string(version_str));
  SPDLOG_DEBUG("max_frame_bytes: {}", ogg_opus_para_->max_frame_bytes_);
  SPDLOG_DEBUG("bitrate: {}", ogg_opus_para_->bitrate);
  SPDLOG_DEBUG("complexity: {}", ogg_opus_para_->complexity);
  SPDLOG_DEBUG("max_ogg_delay: {}", ogg_opus_para_->max_ogg_delay);
}

int OggOpusEncoder::Create(uint32_t sample_rate, uint32_t channels) {
  std::unique_lock<decltype(lock_)> auto_lock(lock_);

  if (ogg_opus_para_ != nullptr) {
    SPDLOG_ERROR("Encoder already created");
    return kOggOpusInvalidState;
  }

  ogg_opus_para_ = new OggOpusEncoderPara();
  if (nullptr == ogg_opus_para_) {
    SPDLOG_ERROR("Failed to allocate OggOpusEncoderPara");
    return kOggOpusCreateFailed;
  }

  is_first_frame_processed_ = false;
  channels_ = channels;
  sample_rate_ = sample_rate;
  frame_sample_num_ = sample_rate_ * frame_duration_ms_ / 1000;
  frame_sample_bytes_ = frame_sample_num_ * channels_ * sizeof(int16_t);

  SPDLOG_DEBUG("sample_rate: {}Hz", sample_rate_);
  SPDLOG_DEBUG("channels: {}", channels_);
  SPDLOG_DEBUG("frame_duration_ms: {}ms", frame_duration_ms_);
  SPDLOG_DEBUG("frame_sample_num: {}", frame_sample_num_);
  SPDLOG_DEBUG("frame_sample_bytes: {}", frame_sample_bytes_);
  SPDLOG_DEBUG("encoder_bitrate: {}", enc_bitrate_);
  SPDLOG_DEBUG("encoder_complexity: {}", enc_complexity_);
  SPDLOG_DEBUG("enable_vbr: {}", enable_vbr_ ? "true" : "false");
  SPDLOG_DEBUG("enable_constraint_vbr: {}", enable_constraint_vbr_ ? "true" : "false");

  ResetParameters();

  // 初始化注释头
  ogg_opus_para_->InitComment();
  ogg_opus_para_->AddComment();
  if (!user_comment_.empty()) {
    ogg_opus_para_->AddUserComment(user_comment_);
  }

  // 设置 Opus 头
  ogg_opus_para_->header.version = 1;  // RFC 7845
  ogg_opus_para_->header.channels = channels_;
  ogg_opus_para_->header.input_sample_rate = sample_rate_;
  ogg_opus_para_->header.gain = 0;

  if (channels_ <= 2) {
    // 单声道或立体声：使用 channel_mapping=0
    ogg_opus_para_->header.channel_mapping = 0;
    ogg_opus_para_->header.nb_streams = 1;
    ogg_opus_para_->header.nb_coupled = (channels_ == 1) ? 0 : 1;
    std::memset(ogg_opus_para_->header.stream_map, 0,
                sizeof(ogg_opus_para_->header.stream_map));
    for (uint32_t i = 0; i < channels_; i++) {
      ogg_opus_para_->header.stream_map[i] = i;
    }
  } else {
    // 多通道：使用 channel_mapping=1（Vorbis 映射）
    ogg_opus_para_->header.channel_mapping = 1;

    if (channels_ == 3) {
      // 3.0: L, R, C
      ogg_opus_para_->header.nb_streams = 2;
      ogg_opus_para_->header.nb_coupled = 1;
      ogg_opus_para_->header.stream_map[0] = 0;  // L
      ogg_opus_para_->header.stream_map[1] = 1;  // R
      ogg_opus_para_->header.stream_map[2] = 2;  // C
    } else if (channels_ == 4) {
      // Quad: FL, FR, RL, RR
      ogg_opus_para_->header.nb_streams = 2;
      ogg_opus_para_->header.nb_coupled = 2;
      ogg_opus_para_->header.stream_map[0] = 0;  // FL
      ogg_opus_para_->header.stream_map[1] = 1;  // FR
      ogg_opus_para_->header.stream_map[2] = 2;  // RL
      ogg_opus_para_->header.stream_map[3] = 3;  // RR
    } else if (channels_ == 6) {
      // 5.1: FL, FR, FC, LFE, RL, RR
      ogg_opus_para_->header.nb_streams = 4;
      ogg_opus_para_->header.nb_coupled = 2;
      ogg_opus_para_->header.stream_map[0] = 0;  // FL
      ogg_opus_para_->header.stream_map[1] = 1;  // FR
      ogg_opus_para_->header.stream_map[2] = 2;  // FC
      ogg_opus_para_->header.stream_map[3] = 3;  // LFE
      ogg_opus_para_->header.stream_map[4] = 4;  // RL
      ogg_opus_para_->header.stream_map[5] = 5;  // RR
    } else {
      // 非标准通道数：保守处理（全部单声道）
      ogg_opus_para_->header.nb_streams = channels_;
      ogg_opus_para_->header.nb_coupled = 0;
      for (uint32_t i = 0; i < channels_; i++) {
        ogg_opus_para_->header.stream_map[i] = i;
      }
    }
  }

  SPDLOG_DEBUG("nb_streams: {}", ogg_opus_para_->header.nb_streams);
  SPDLOG_DEBUG("nb_coupled: {}", ogg_opus_para_->header.nb_coupled);

  // 创建 Opus 多流编码器
  int application = (app_type_ == "AUDIO") ? OPUS_APPLICATION_AUDIO : OPUS_APPLICATION_VOIP;
  int ret = OPUS_OK;

  ogg_opus_para_->opus_ms_encoder_ = opus_multistream_encoder_create(
      sample_rate_, channels_, ogg_opus_para_->header.nb_streams,
      ogg_opus_para_->header.nb_coupled, ogg_opus_para_->header.stream_map,
      application, &ret);

  if (ret != OPUS_OK) {
    SPDLOG_ERROR("Cannot create encoder: {}", opus_strerror(ret));
    return kOpusEncoderCreateFailed;
  }

  SPDLOG_DEBUG("opus_multistream_encoder_create success. sample_rate:{} "
               "channels:{}, application:{}",
               sample_rate_, channels_, application);

  // 分配编码数据包缓冲区
  ogg_opus_para_->packet_ = new uint8_t[ogg_opus_para_->max_frame_bytes_]{};
  if (nullptr == ogg_opus_para_->packet_) {
    SPDLOG_ERROR("Failed to allocate packet buffer");
    return kMemAllocError;
  }

  SPDLOG_DEBUG("nb_streams {}, nb_coupled {}, bitrate {}, max frame bytes: {}",
               ogg_opus_para_->header.nb_streams,
               ogg_opus_para_->header.nb_coupled, ogg_opus_para_->bitrate,
               ogg_opus_para_->max_frame_bytes_);

  // 配置编码器参数
  ret = opus_multistream_encoder_ctl(ogg_opus_para_->opus_ms_encoder_,
                                     OPUS_SET_BITRATE(ogg_opus_para_->bitrate));
  if (ret != OPUS_OK) {
    SPDLOG_ERROR("OPUS_SET_BITRATE failed: {}", opus_strerror(ret));
    return kOggOpusEncodeFailed;
  }

  ret = opus_multistream_encoder_ctl(ogg_opus_para_->opus_ms_encoder_,
                                     OPUS_SET_VBR(enable_vbr_ ? 1 : 0));
  if (ret != OPUS_OK) {
    SPDLOG_ERROR("OPUS_SET_VBR failed: {}", opus_strerror(ret));
    return kOggOpusEncodeFailed;
  }

  ret = opus_multistream_encoder_ctl(
      ogg_opus_para_->opus_ms_encoder_,
      OPUS_SET_VBR_CONSTRAINT(enable_constraint_vbr_ ? 1 : 0));
  if (ret != OPUS_OK) {
    SPDLOG_ERROR("OPUS_SET_VBR_CONSTRAINT failed: {}", opus_strerror(ret));
    return kOggOpusEncodeFailed;
  }

  ret = opus_multistream_encoder_ctl(
      ogg_opus_para_->opus_ms_encoder_,
      OPUS_SET_COMPLEXITY(ogg_opus_para_->complexity));
  if (ret != OPUS_OK) {
    SPDLOG_ERROR("OPUS_SET_COMPLEXITY failed: {}", opus_strerror(ret));
    return kOggOpusEncodeFailed;
  }

  ret = opus_multistream_encoder_ctl(ogg_opus_para_->opus_ms_encoder_,
                                     OPUS_SET_PACKET_LOSS_PERC(0));
  if (ret != OPUS_OK) {
    SPDLOG_ERROR("OPUS_SET_PACKET_LOSS_PERC failed: {}", opus_strerror(ret));
    return kOggOpusEncodeFailed;
  }

#ifdef OPUS_SET_LSB_DEPTH
  ret = opus_multistream_encoder_ctl(
      ogg_opus_para_->opus_ms_encoder_,
      OPUS_SET_LSB_DEPTH(ogg_opus_para_->ogg_encode_opt.sample_bits));
  if (ret != OPUS_OK) {
    SPDLOG_WARN("OPUS_SET_LSB_DEPTH failed: {}", opus_strerror(ret));
  }
#endif

  // 获取 lookahead 值
  ret = opus_multistream_encoder_ctl(ogg_opus_para_->opus_ms_encoder_,
                                     OPUS_GET_LOOKAHEAD(&ogg_opus_para_->lookahead));
  if (ret != OPUS_OK) {
    SPDLOG_ERROR("OPUS_GET_LOOKAHEAD failed: {}", opus_strerror(ret));
    return kOggOpusEncodeFailed;
  }

  ogg_opus_para_->ogg_encode_opt.skip += ogg_opus_para_->lookahead;
  ogg_opus_para_->header.preskip = ogg_opus_para_->ogg_encode_opt.skip;

  // 初始化 Ogg 流
  if (ogg_stream_init(&ogg_opus_para_->os, ogg_opus_para_->serialno) == -1) {
    SPDLOG_ERROR("Ogg stream init failed");
    return kOggInitFailed;
  }

  // 分配输入缓冲区（float 格式）
  ogg_opus_para_->input_pcm_ = new float[frame_sample_num_ * channels_]{};
  if (ogg_opus_para_->input_pcm_ == nullptr) {
    SPDLOG_ERROR("Failed to allocate input buffer");
    return kMemAllocError;
  }

  // 准备编码
  ogg_opus_para_->op.e_o_s = 0;
  ogg_opus_para_->nb_samples = 0;
  is_first_frame_processed_ = false;

  return kSuccess;
}

int OggOpusEncoder::Destroy() {
  std::unique_lock<decltype(lock_)> auto_lock(lock_);

  if (nullptr == ogg_opus_para_) {
    return kOggOpusInvalidState;
  }

  // 释放注释
  if (ogg_opus_para_->ogg_encode_opt.comments) {
    delete[] ogg_opus_para_->ogg_encode_opt.comments;
    ogg_opus_para_->ogg_encode_opt.comments = nullptr;
  }

  // 销毁编码器
  if (ogg_opus_para_->opus_ms_encoder_) {
    opus_multistream_encoder_destroy(ogg_opus_para_->opus_ms_encoder_);
    ogg_opus_para_->opus_ms_encoder_ = nullptr;
  }

  // 清除 Ogg 流
  ogg_stream_clear(&ogg_opus_para_->os);

  // 释放缓冲区
  if (ogg_opus_para_->packet_) {
    delete[] ogg_opus_para_->packet_;
    ogg_opus_para_->packet_ = nullptr;
  }

  if (ogg_opus_para_->input_pcm_) {
    delete[] ogg_opus_para_->input_pcm_;
    ogg_opus_para_->input_pcm_ = nullptr;
  }

  // 清空缓存
  ogg_opus_para_->cache_pcm16_.clear();
  ogg_opus_para_->cache_pcm24_.clear();
  ogg_opus_para_->cache_pcm_.clear();

  delete ogg_opus_para_;
  ogg_opus_para_ = nullptr;

  is_first_frame_processed_ = false;

  return kSuccess;
}

int OggOpusEncoder::Encode(const int16_t* pcm_data, size_t frame_size,
                           uint8_t* encoded_buf, size_t encoded_buf_cap,
                           size_t& encoded_size, bool is_eof) {
  std::unique_lock<decltype(lock_)> auto_lock(lock_);
  encoded_size = 0;

  if (!ogg_opus_para_ || !pcm_data || !encoded_buf) {
    return kOggOpusInvalidState;
  }

  const size_t total_samples = frame_size * channels_;

  // 将 PCM16 数据写入缓存
  WritePcm16ToCache(pcm_data, total_samples, ogg_opus_para_->cache_pcm16_);

  // 检查是否有足够的数据进行编码
  if (ogg_opus_para_->cache_pcm16_.size() < frame_sample_num_ * channels_) {
    return kSuccess;  // 数据不足，等待更多数据
  }

  // 从缓存读取一帧数据并转换为 float
  const size_t samples_read = ReadPcm16FromCache(
      ogg_opus_para_->cache_pcm16_, ogg_opus_para_->input_pcm_,
      frame_sample_num_ * channels_);

  if (samples_read < frame_sample_num_ * channels_) {
    SPDLOG_WARN("Insufficient samples read: {} < {}", samples_read,
                frame_sample_num_ * channels_);
    return kOggOpusEncodeFailed;
  }

  // 执行编码
  return EncodeInner(ogg_opus_para_->input_pcm_, samples_read, encoded_buf,
                     encoded_buf_cap, encoded_size, is_eof, 32);
}

int OggOpusEncoder::Encode24(const int32_t* pcm_data, size_t frame_size,
                             uint8_t* encoded_buf, size_t encoded_buf_cap,
                             size_t& encoded_size, bool is_eof) {
  std::unique_lock<decltype(lock_)> auto_lock(lock_);
  encoded_size = 0;

  if (!ogg_opus_para_ || !pcm_data || !encoded_buf) {
    return kOggOpusInvalidState;
  }

  const size_t total_samples = frame_size * channels_;

  // 将 PCM24 数据写入缓存
  WritePcm24ToCache(pcm_data, total_samples, ogg_opus_para_->cache_pcm24_);

  // 检查是否有足够的数据进行编码
  if (ogg_opus_para_->cache_pcm24_.size() < frame_sample_num_ * channels_) {
    return kSuccess;  // 数据不足，等待更多数据
  }

  // 从缓存读取一帧数据并转换为 float
  const size_t samples_read = ReadPcm24FromCache(
      ogg_opus_para_->cache_pcm24_, ogg_opus_para_->input_pcm_,
      frame_sample_num_ * channels_);

  if (samples_read < frame_sample_num_ * channels_) {
    SPDLOG_WARN("Insufficient samples read: {} < {}", samples_read,
                frame_sample_num_ * channels_);
    return kOggOpusEncodeFailed;
  }

  // 执行编码
  return EncodeInner(ogg_opus_para_->input_pcm_, samples_read, encoded_buf,
                     encoded_buf_cap, encoded_size, is_eof, 32);
}

int OggOpusEncoder::EncodeFloat(const float* pcm_data, size_t frame_size,
                                uint8_t* encoded_buf, size_t encoded_buf_cap,
                                size_t& encoded_size, bool is_eof) {
  std::unique_lock<decltype(lock_)> auto_lock(lock_);
  encoded_size = 0;

  if (!ogg_opus_para_ || !pcm_data || !encoded_buf) {
    return kOggOpusInvalidState;
  }

  const size_t total_samples = frame_size * channels_;

  // 将 PCM32(float) 数据写入缓存
  WritePcm32ToCache(pcm_data, total_samples, ogg_opus_para_->cache_pcm_);

  // 检查是否有足够的数据进行编码
  if (ogg_opus_para_->cache_pcm_.size() < frame_sample_num_ * channels_) {
    return kSuccess;  // 数据不足，等待更多数据
  }

  // 从缓存读取一帧数据
  const size_t samples_read = ReadPcm32FromCache(
      ogg_opus_para_->cache_pcm_, ogg_opus_para_->input_pcm_,
      frame_sample_num_ * channels_);

  if (samples_read < frame_sample_num_ * channels_) {
    SPDLOG_WARN("Insufficient samples read: {} < {}", samples_read,
                frame_sample_num_ * channels_);
    return kOggOpusEncodeFailed;
  }

  // 执行编码
  return EncodeInner(ogg_opus_para_->input_pcm_, samples_read, encoded_buf,
                     encoded_buf_cap, encoded_size, is_eof, 32);
}

int OggOpusEncoder::Flush(uint8_t* encoded_buf, size_t encoded_buf_cap,
                          size_t& encoded_size, bool is_eof, int depth) {
  std::unique_lock<decltype(lock_)> auto_lock(lock_);
  encoded_size = 0;

  if (!ogg_opus_para_) {
    return kOggOpusInvalidState;
  }

  // 检查是否有剩余数据
  size_t remaining_samples = 0;
  if (depth == 16) {
    remaining_samples = ogg_opus_para_->cache_pcm16_.size();
  } else if (depth == 24) {
    remaining_samples = ogg_opus_para_->cache_pcm24_.size();
  } else if (depth == 32) {
    remaining_samples = ogg_opus_para_->cache_pcm_.size();
  }

  if (remaining_samples == 0) {
    return kSuccess;
  }

  // 读取剩余数据
  size_t samples_read = 0;
  if (depth == 16) {
    samples_read = ReadPcm16FromCache(ogg_opus_para_->cache_pcm16_,
                                      ogg_opus_para_->input_pcm_,
                                      frame_sample_num_ * channels_);
  } else if (depth == 24) {
    samples_read = ReadPcm24FromCache(ogg_opus_para_->cache_pcm24_,
                                      ogg_opus_para_->input_pcm_,
                                      frame_sample_num_ * channels_);
  } else if (depth == 32) {
    samples_read = ReadPcm32FromCache(ogg_opus_para_->cache_pcm_,
                                      ogg_opus_para_->input_pcm_,
                                      frame_sample_num_ * channels_);
  }

  if (samples_read == 0) {
    return kSuccess;
  }

  // 编码剩余数据
  return EncodeInner(ogg_opus_para_->input_pcm_, samples_read, encoded_buf,
                     encoded_buf_cap, encoded_size, is_eof, 32);
}

int OggOpusEncoder::SoftReset() {
  std::unique_lock<decltype(lock_)> auto_lock(lock_);
  is_first_frame_processed_ = false;
  ogg_stream_reset(&ogg_opus_para_->os);

  // 清空缓存
  ogg_opus_para_->cache_pcm16_.clear();
  ogg_opus_para_->cache_pcm24_.clear();
  ogg_opus_para_->cache_pcm_.clear();

  return kSuccess;
}

size_t OggOpusEncoder::RemainingFrames(int depth) {
  if (!ogg_opus_para_) {
    return 0;
  }

  size_t cached_samples = 0;
  if (depth == 16) {
    cached_samples = ogg_opus_para_->cache_pcm16_.size();
  } else if (depth == 24) {
    cached_samples = ogg_opus_para_->cache_pcm24_.size();
  } else if (depth == 32) {
    cached_samples = ogg_opus_para_->cache_pcm_.size();
  }

  const size_t samples_per_frame = frame_sample_num_ * channels_;
  return (samples_per_frame > 0) ? (cached_samples / samples_per_frame) : 0;
}

int OggOpusEncoder::EncodeInner(const void* pcm_data, size_t total_samples,
                                uint8_t* encoded_data, size_t encoded_buf_cap,
                                size_t& encoded_size, bool is_eof, int depth) {
  if (!ogg_opus_para_ || !pcm_data || !encoded_data) {
    return kOggOpusInvalidState;
  }

  // 每通道样本数
  const size_t samples_per_channel = total_samples / channels_;
  const int max_output_bytes =
      (encoded_buf_cap > static_cast<size_t>(ogg_opus_para_->max_frame_bytes_))
          ? encoded_buf_cap
          : ogg_opus_para_->max_frame_bytes_;

  int bytes_encoded = 0;

  // 根据深度选择编码函数
  if (depth == 16) {
    // PCM16 直接使用 opus_multistream_encode
    bytes_encoded = opus_multistream_encode(
        ogg_opus_para_->opus_ms_encoder_,
        static_cast<const opus_int16*>(pcm_data), samples_per_channel,
        encoded_data, max_output_bytes);
  } else if (depth == 24) {
    // PCM24 直接使用 opus_multistream_encode24
    bytes_encoded = opus_multistream_encode24(
        ogg_opus_para_->opus_ms_encoder_,
        static_cast<const opus_int32*>(pcm_data), samples_per_channel,
        encoded_data, max_output_bytes);
  } else if (depth == 32) {
    // PCM32 (float) 使用 opus_multistream_encode_float
    bytes_encoded = opus_multistream_encode_float(
        ogg_opus_para_->opus_ms_encoder_,
        static_cast<const float*>(pcm_data), samples_per_channel, encoded_data,
        max_output_bytes);
  } else {
    SPDLOG_ERROR("Invalid PCM depth: {}", depth);
    return kInvalidInputParams;
  }

  if (bytes_encoded < 0) {
    SPDLOG_ERROR("Encoding failed: {} ({})", bytes_encoded,
                 opus_strerror(bytes_encoded));
    return kOggOpusEncodeFailed;
  }

  // 更新 granule position
  const uint64_t next_granulepos =
      ogg_opus_para_->enc_granulepos + samples_per_channel;

  // 准备 Ogg 包
  ogg_opus_para_->op.granulepos = next_granulepos;
  ogg_opus_para_->op.packet = encoded_data;
  ogg_opus_para_->op.bytes = bytes_encoded;
  ogg_opus_para_->op.b_o_s = 0;
  ogg_opus_para_->op.e_o_s = is_eof ? 1 : 0;
  ogg_opus_para_->op.packetno = ogg_opus_para_->id++;

  if (debug_) {
    SPDLOG_DEBUG("ogg_packet granulepos:{}, bytes:{}, packetno:{}",
                 ogg_opus_para_->op.granulepos, ogg_opus_para_->op.bytes,
                 ogg_opus_para_->op.packetno);
  }

  // 将包添加到 Ogg 流
  ogg_stream_packetin(&ogg_opus_para_->os, &ogg_opus_para_->op);

  // 从 Ogg 流中提取页面
  while (ogg_stream_flush(&ogg_opus_para_->os, &ogg_opus_para_->og)) {
    // 调用回调函数发送编码后的数据
    int written_ret = ogg_opus_para_->WritePage();
    if (written_ret !=
        static_cast<int>(ogg_opus_para_->og.header_len + ogg_opus_para_->og.body_len)) {
      SPDLOG_ERROR("Failed writing data to output stream");
      return kOggOpusEncodeFailed;
    }
  }

  // 更新 granule 位置
  ogg_opus_para_->enc_granulepos = next_granulepos;

  encoded_size = bytes_encoded;
  return kSuccess;
}

void OggOpusEncoder::SetSampleRate(int sample_rate) {
  sample_rate_ = sample_rate;
  frame_sample_num_ = sample_rate_ * frame_duration_ms_ / 1000;
  frame_sample_bytes_ = frame_sample_num_ * channels_ * (sample_bits_ / 8);

  if (ogg_opus_para_) {
    ogg_opus_para_->max_frame_bytes_ = frame_sample_bytes_ * 2;

    // 重新分配输入缓冲区
    if (ogg_opus_para_->input_pcm_) {
      delete[] ogg_opus_para_->input_pcm_;
    }
    ogg_opus_para_->input_pcm_ = new float[frame_sample_num_ * channels_]{};

    // 重新分配输出缓冲区
    if (ogg_opus_para_->packet_) {
      delete[] ogg_opus_para_->packet_;
    }
    ogg_opus_para_->packet_ = new uint8_t[ogg_opus_para_->max_frame_bytes_]{};

    SPDLOG_DEBUG(
        "Reset sample_rate is {}, frame_sample_num is {}, "
        "frame_sample_bytes is {}, max_frame_bytes is {}",
        sample_rate_, frame_sample_num_, frame_sample_bytes_,
        ogg_opus_para_->max_frame_bytes_);
  }
}

int OggOpusEncoder::SetFrameSampleBytes(uint32_t bytes) {
  if (bytes == static_cast<int>(frame_sample_bytes_)) {
    return kSuccess;
  }

  const int item_bytes =
      sample_rate_ / 100 * (sample_bits_ / 8) * channels_;  // 10ms

  // 验证可设置的对应音频长度为 10ms, 20ms, 40ms, 60ms, 100ms, 120ms
  if (bytes % item_bytes != 0 ||
      (bytes != item_bytes && bytes != item_bytes * 2 &&
       bytes != item_bytes * 4 && bytes != item_bytes * 6 &&
       bytes != item_bytes * 10 && bytes != item_bytes * 12)) {
    SPDLOG_ERROR("Frame sample bytes {} is invalid!", bytes);
    return kOpusInvalidParameter;
  }

  frame_sample_bytes_ = bytes;
  frame_sample_num_ = bytes / (sample_bits_ / 8) / channels_;

  if (ogg_opus_para_) {
    if (frame_sample_bytes_ > ogg_opus_para_->max_frame_bytes_) {
      ogg_opus_para_->max_frame_bytes_ = frame_sample_bytes_ * 2;
      if (ogg_opus_para_->packet_) {
        delete[] ogg_opus_para_->packet_;
      }
      ogg_opus_para_->packet_ = new uint8_t[ogg_opus_para_->max_frame_bytes_]{};
    }

    if (ogg_opus_para_->input_pcm_) {
      delete[] ogg_opus_para_->input_pcm_;
    }
    ogg_opus_para_->input_pcm_ = new float[frame_sample_num_ * channels_]{};

    if (debug_) {
      SPDLOG_DEBUG(
          "Reset frame_sample_num is {}, frame_sample_bytes is {}, "
          "max_frame_bytes is {}",
          frame_sample_num_, frame_sample_bytes_,
          ogg_opus_para_->max_frame_bytes_);
    }
  }

  return kSuccess;
}

void OggOpusEncoder::SetChannelNum(int ch) {
  channels_ = ch;
  frame_sample_num_ = sample_rate_ * frame_duration_ms_ / 1000;
  frame_sample_bytes_ = frame_sample_num_ * channels_ * (sample_bits_ / 8);

  if (ogg_opus_para_) {
    ogg_opus_para_->max_frame_bytes_ = frame_sample_bytes_ * 2;

    if (ogg_opus_para_->input_pcm_) {
      delete[] ogg_opus_para_->input_pcm_;
    }
    ogg_opus_para_->input_pcm_ = new float[frame_sample_num_ * channels_]{};

    if (ogg_opus_para_->packet_) {
      delete[] ogg_opus_para_->packet_;
    }
    ogg_opus_para_->packet_ = new uint8_t[ogg_opus_para_->max_frame_bytes_]{};

    SPDLOG_DEBUG(
        "Reset channels is {}, frame_sample_num is {}, "
        "frame_sample_bytes is {}, max_frame_bytes is {}",
        channels_, frame_sample_num_, frame_sample_bytes_,
        ogg_opus_para_->max_frame_bytes_);
  }
}

int OggOpusEncoder::SetFrameSize(uint32_t ms) {
  if (frame_duration_ms_ == static_cast<uint32_t>(ms)) {
    return kSuccess;
  }

  // 验证帧大小
  if (ms != 10 && ms != 20 && ms != 40 && ms != 60 && ms != 100 && ms != 120) {
    SPDLOG_ERROR("Frame size {}ms is invalid!", ms);
    return kOpusInvalidParameter;
  }

  frame_duration_ms_ = ms;
  frame_sample_num_ = sample_rate_ * frame_duration_ms_ / 1000;
  frame_sample_bytes_ = frame_sample_num_ * channels_ * (sample_bits_ / 8);

  if (ogg_opus_para_) {
    if (frame_sample_bytes_ > ogg_opus_para_->max_frame_bytes_) {
      ogg_opus_para_->max_frame_bytes_ = frame_sample_bytes_ * 2;
      if (ogg_opus_para_->packet_) {
        delete[] ogg_opus_para_->packet_;
      }
      ogg_opus_para_->packet_ = new uint8_t[ogg_opus_para_->max_frame_bytes_]{};
    }

    if (ogg_opus_para_->input_pcm_) {
      delete[] ogg_opus_para_->input_pcm_;
    }
    ogg_opus_para_->input_pcm_ = new float[frame_sample_num_ * channels_]{};

    if (debug_) {
      SPDLOG_DEBUG(
          "Reset frame_duration_ms is {}, frame_sample_num is {}, "
          "frame_sample_bytes is {}, max_frame_bytes is {}",
          frame_duration_ms_, frame_sample_num_, frame_sample_bytes_,
          ogg_opus_para_->max_frame_bytes_);
    }
  }

  return kSuccess;
}

void OggOpusEncoder::SetDebugMode(bool enable) {
  debug_ = enable;
  if (ogg_opus_para_) {
    ogg_opus_para_->debug = debug_;
  }
}

// ===== 兼容旧版 API 接口实现 =====

int OggOpusEncoder::OggopusEncoderCreate(void* callback, void* user_data,
                                         uint32_t sample_rate,
                                         uint32_t channels) {
  encoded_callback_ = callback;
  callback_user_data_ = user_data;
  return Create(sample_rate, channels);
}

int OggOpusEncoder::OggopusEncode(const char* pcm_data, int data_len) {
  if (!ogg_opus_para_ || !pcm_data || data_len <= 0) {
    return kOggOpusInvalidState;
  }

  // 将 PCM 数据写入缓存
  const size_t total_bytes = static_cast<size_t>(data_len);
  const size_t total_samples = total_bytes / sizeof(int16_t);

  // 写入 PCM16 缓存
  ogg_opus_para_->cache_pcm16_.insert(
      ogg_opus_para_->cache_pcm16_.end(),
      reinterpret_cast<const int16_t*>(pcm_data),
      reinterpret_cast<const int16_t*>(pcm_data) + total_samples);

  // 检查是否有足够的数据进行编码
  const size_t samples_needed = frame_sample_num_ * channels_;
  if (ogg_opus_para_->cache_pcm16_.size() < samples_needed) {
    return kSuccess;  // 数据不足，等待更多数据
  }

  // 从缓存读取一帧数据并转换为 float
  auto input_buffer = std::make_unique<float[]>(samples_needed);
  const size_t samples_read = ReadPcm16FromCache(
      ogg_opus_para_->cache_pcm16_, input_buffer.get(), samples_needed);

  if (samples_read < samples_needed) {
    SPDLOG_WARN("Insufficient samples read: {} < {}", samples_read,
                samples_needed);
    return kOggOpusEncodeFailed;
  }

  // 执行编码
  std::vector<uint8_t> encoded_buf(ogg_opus_para_->max_frame_bytes_);
  size_t encoded_size = 0;

  int ret = EncodeInner(input_buffer.get(), samples_read, encoded_buf.data(),
                        encoded_buf.size(), encoded_size, false, 32);

  if (ret != kSuccess) {
    return ret;
  }

  // 将编码后的数据推入内部缓冲区
  if (encoded_size > 0) {
    encoded_data_buffer_.insert(encoded_data_buffer_.end(),
                                encoded_buf.begin(),
                                encoded_buf.begin() + encoded_size);
  }

  return kSuccess;
}

int OggOpusEncoder::OggopusGetOuputSize() {
  return static_cast<int>(encoded_data_buffer_.size());
}

int OggOpusEncoder::OggopusGetOuput(unsigned char* output_buf, int buf_size) {
  if (!output_buf || buf_size <= 0) {
    return 0;
  }

  const size_t available = encoded_data_buffer_.size();
  const size_t to_copy = std::min(static_cast<size_t>(buf_size), available);

  if (to_copy > 0) {
    std::memcpy(output_buf, encoded_data_buffer_.data(), to_copy);
    // 移除已读取的数据
    encoded_data_buffer_.erase(encoded_data_buffer_.begin(),
                               encoded_data_buffer_.begin() + to_copy);
  }

  return static_cast<int>(to_copy);
}

int OggOpusEncoder::OggopusPushEncodedData(const uint8_t* encoded_data,
                                           int data_len) {
  if (!encoded_data || data_len <= 0) {
    return kInvalidInputParams;
  }

  // 如果有回调函数，则调用回调
  if (encoded_callback_) {
    using CallbackType = size_t (*)(const uint8_t*, int, void*);
    auto callback = reinterpret_cast<CallbackType>(encoded_callback_);
    callback(encoded_data, data_len, callback_user_data_);
  }

  // 同时也存入内部缓冲区
  encoded_data_buffer_.insert(encoded_data_buffer_.end(),
                              encoded_data,
                              encoded_data + data_len);

  return kSuccess;
}

int OggOpusEncoder::OggopusSoftRestart() {
  return SoftReset();
}

int OggOpusEncoder::OggopusDestroy() {
  return Destroy();
}

}  // namespace mproc

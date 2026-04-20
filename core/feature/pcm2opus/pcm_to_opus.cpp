/*
 * Copyright (c) 2026 season(season4675@gmail.com). All rights reserved.
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#include "pcm2opus/pcm_to_opus.h"

#include <cstddef>
#include <string>

#include "spdlog/spdlog.h"

namespace mproc {

PcmToOpus::PcmToOpus(MediaContext* ctx_handler)
    : ctx_handler_(ctx_handler),
      encoder_(nullptr),
      channels_(1),
      sample_rate_(16000),
      frame_size_(-1),
      frame_sample_bytes_(0),
      debug_(false),
      vbr_(true) {
  name_ = "PcmToOpus";
}

PcmToOpus::~PcmToOpus() {
  SPDLOG_INFO("destructing PcmToOpus:[{}]", static_cast<void*>(this));
  Destroy();
}

MediaStatus PcmToOpus::Create(MediaContext& context) {
  std::unique_lock<decltype(this->encoder_lock_)> auto_lock(
      this->encoder_lock_);

  this->channels_ =
      context.Output().GetChannels() > 0 ? context.Output().GetChannels() : 1;
  this->sample_rate_ = context.Output().GetSampleRate() > 0
                           ? context.Output().GetSampleRate()
                           : 16000;

  // special parameters
  this->frame_size_ = context.Output().GetOptInt("frame_size");
  int bitrate = context.Output().GetOptInt("bitrate");
  if (bitrate > 0) {
    this->bitrate_ = std::to_string(bitrate);
  }
  int complexity = context.Output().GetOptInt("complexity", -1);
  if (complexity >= 0) {
    this->complexity_ = std::to_string(complexity);
  }
  this->vbr_ = context.Output().GetOptBool("vbr", true);
  this->application_ = context.Output().GetOptStr("application", "");
  this->user_comment_ = context.Output().GetOptStr("comment", "");
  this->debug_ = context.Input().GetOptBool("debug", false) ||
                 context.Output().GetOptBool("debug", false);

#ifdef MP_INCLUDE_PROFILE
  if (this->debug_) {
    SPDLOG_DEBUG("Start profiler ...");
    this->profile_.StartRoundTiming();
  }
#endif

  return CreateLocked();
}

MediaStatus PcmToOpus::Destroy() {
  std::unique_lock<decltype(this->encoder_lock_)> auto_lock(
      this->encoder_lock_);
#ifdef MP_INCLUDE_PROFILE
  if (this->debug_) {
    this->profile_.StopRoundTiming(this->name_.c_str());
  }
#endif
  return DestroyLocked();
}

MediaStatus PcmToOpus::Process(MediaPacket& input, MediaConfig& src_frame,
                               MediaPacket& output, MediaConfig& dst_frame,
                               bool& update) {
  std::unique_lock<decltype(this->encoder_lock_)> auto_lock(this->encoder_lock_);
  
  output.SetSize(0, 0);
  if (input.GetData(0) == nullptr || input.GetSize(0) == 0) {
    SPDLOG_ERROR("Invalid input data");
    return kInvalidInputParams;
  }

  if (!encoder_) {
    SPDLOG_WARN("Encoder not initialized");
    return kEncoderInexistent;
  }

  auto* opus_encoder = static_cast<OggOpusEncoder*>(encoder_);
  const uint8_t* pcm_data = static_cast<const uint8_t*>(input.GetData(0));
  size_t input_bytes = input.GetSize(0);
  
  // 计算 PCM16 样本数（每个样本 2 字节）
  size_t total_samples = input_bytes / sizeof(int16_t);
  
  SPDLOG_TRACE("[ENC:{}] Input {} bytes ({} samples)",
               static_cast<void*>(encoder_), input_bytes, total_samples);

#ifdef MP_INCLUDE_PROFILE
  if (this->debug_) {
    this->profile_.StartElementTiming();
  }
#endif

  // 预分配输出缓冲区（Opus 编码后通常小于原始 PCM，分配 2 倍安全系数）
  size_t max_output_bytes = input_bytes * 2;
  if (output.GetCapacity(0) < max_output_bytes) {
    output.SetBuffer(new uint8_t[max_output_bytes], max_output_bytes, 0, 0,
                     [](void* p) { delete[] static_cast<uint8_t*>(p); });
  }
  
  // 调用 Encode 接口进行编码
  size_t encoded_size = 0;
  const int16_t* pcm_frame = reinterpret_cast<const int16_t*>(pcm_data);
  int ret = opus_encoder->Encode(pcm_frame, total_samples, 
                                 static_cast<uint8_t*>(const_cast<void*>(output.GetData(0))), 
                                 output.GetCapacity(0), 
                                 encoded_size);
  
  if (ret != kSuccess) {
    SPDLOG_ERROR("[ENC:{}] Encode failed, ret={}", static_cast<void*>(encoder_), ret);
    return kOggOpusEncodeFailed;
  }
  
  // 设置编码后的数据大小
  if (encoded_size > 0) {
    output.SetSize(0, encoded_size);
    SPDLOG_TRACE("[ENC:{}] Encoded {} bytes", static_cast<void*>(encoder_), encoded_size);
  }

#ifdef MP_INCLUDE_PROFILE
  if (this->debug_) {
    this->profile_.StopElementTiming(this->name_.c_str());
  }
#endif

  return kSuccess;
}

MediaStatus PcmToOpus::Reset() {
  std::unique_lock<decltype(this->encoder_lock_)> auto_lock(this->encoder_lock_);
  
  SPDLOG_DEBUG("Reset OggOpusEncoder...");
  if (!encoder_) {
    SPDLOG_WARN("Encoder not initialized");
    return kEncoderInexistent;
  }

  auto* opus_encoder = static_cast<OggOpusEncoder*>(encoder_);
  opus_encoder->SoftReset();
  
  return kSuccess;
}

MediaStatus PcmToOpus::CreateLocked() {
  if (this->encoder_) {
    static_cast<OggOpusEncoder*>(encoder_)->Destroy();
    delete encoder_;
    encoder_ = nullptr;
  }

  this->encoder_ = new OggOpusEncoder();
  auto* opus_encoder = static_cast<OggOpusEncoder*>(encoder_);

  // 配置编码器参数
  if (this->frame_size_ > 0) {
    SPDLOG_DEBUG("Set frame size {}", this->frame_size_);
    opus_encoder->SetFrameSize(this->frame_size_);
  }
  if (!this->bitrate_.empty()) {
    SPDLOG_DEBUG("Set bitrate {}", this->bitrate_);
    opus_encoder->SetBitrate(std::stoi(this->bitrate_));
  }
  if (!this->complexity_.empty()) {
    SPDLOG_DEBUG("Set complexity {}", this->complexity_);
    opus_encoder->SetComplexity(std::stoi(this->complexity_));
  }
  SPDLOG_DEBUG("Set VBR {}", this->vbr_);
  opus_encoder->SetVBRMode(this->vbr_);
  if (!this->application_.empty()) {
    std::string app_str = this->application_;
    std::transform(app_str.begin(), app_str.end(), app_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    SPDLOG_DEBUG("Set application type {}", app_str);
    opus_encoder->SetApplicationType(app_str);
  }
  if (!this->user_comment_.empty()) {
    SPDLOG_DEBUG("Set user comment {}", this->user_comment_);
    opus_encoder->AddUserComment(this->user_comment_);
  }

  SPDLOG_DEBUG("Set Debug mode {}", this->debug_ ? "true" : "false");
  opus_encoder->SetDebugMode(this->debug_);

  // 使用新的 Create 接口
  int ret = opus_encoder->Create(this->sample_rate_, this->channels_);
  if (ret == kSuccess) {
    SPDLOG_DEBUG("OggOpusEncoder created, sample_rate={}, channels={}",
                 this->sample_rate_, this->channels_);
    this->frame_sample_bytes_ = opus_encoder->GetFrameSampleBytes();
  } else {
    SPDLOG_ERROR("Create failed, error={}", ret);
  }

  return static_cast<MediaStatus>(ret);
}

MediaStatus PcmToOpus::DestroyLocked() {
  if (this->encoder_) {
    static_cast<OggOpusEncoder*>(encoder_)->Destroy();
    delete this->encoder_;
    this->encoder_ = nullptr;
  }
  SPDLOG_DEBUG("OggOpusEncoder destroyed");
  return kSuccess;
}

}  // namespace mproc
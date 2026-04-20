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

static size_t OggopusEncodedData(const uint8_t* encoded_data, int len,
                                 void* user_data) {
  PcmToOpus* encoder = reinterpret_cast<PcmToOpus*>(user_data);
  if (encoder && encoded_data) {
    encoder->pushback_encoded_data(encoded_data, len);
  }
  return len;
}

int PcmToOpus::pushback_encoded_data(const uint8_t* encoded_data, int data_len) {
  (static_cast<OggOpusEncoder*>(encoder_))
      ->OggopusPushEncodedData(encoded_data, data_len);
  return kSuccess;
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

  return create_locked();
}

MediaStatus PcmToOpus::Destroy() {
  std::unique_lock<decltype(this->encoder_lock_)> auto_lock(
      this->encoder_lock_);
#ifdef MP_INCLUDE_PROFILE
  if (this->debug_) {
    this->profile_.StopRoundTiming(this->name_.c_str());
  }
#endif
  return destroy_locked();
}

MediaStatus PcmToOpus::Process(MediaPacket& input, MediaConfig& src_frame,
                               MediaPacket& output, MediaConfig& dst_frame,
                               bool& update) {
  std::unique_lock<decltype(this->encoder_lock_)> auto_lock(
      this->encoder_lock_);
  output.SetSize(0, 0);
  if (input.GetData(0) == nullptr) {
    SPDLOG_ERROR("invalid nullptr data of input!");
    return kInvalidInputParams;
  }
  if (input.GetCapacity(0) == 0) {
    SPDLOG_ERROR("invalid zero bytes of intput.data!");
    return kInvalidInputParams;
  }
  if (input.GetSize(0) == 0) {
    input.SetSize(0, input.GetCapacity(0));
  }

  if (!encoder_) {
    SPDLOG_WARN("audio_decoder is inexistent in AudioDecoding.");
    return kDecoderInexistent;
  }

  int predicted_capacity = input.GetSize(0);
  if (output.GetData(0) && output.GetCapacity(0) < predicted_capacity) {
    delete[] (uint8_t*)output.GetData(0);
    output.SetBuffer(nullptr, 0, 0, 0);
  }
  if (output.GetData(0) == nullptr) {
    SPDLOG_TRACE("New buffer({}bytes) with sample rate:{} and channels:{}.",
                 predicted_capacity, this->sample_rate_, this->channels_);
    output.SetBuffer(new uint8_t[predicted_capacity], predicted_capacity, 0, 0,
                     [](void* p) { delete[] (uint8_t*)p; });
  }

  SPDLOG_TRACE(
      "[ENC:{}] Frame sample should {}bytes, and input {}bytes, and {}bytes in "
      "cache.",
      static_cast<void*>(encoder_), frame_sample_bytes_, input.GetSize(0), cache_.size());

#ifdef MP_INCLUDE_PROFILE
  if (this->debug_) {
    this->profile_.StartElementTiming();
  }
#endif

  int erase_bytes = 0;
  char* input_buf = (char*)input.GetData(0);
  int input_bytes = frame_sample_bytes_;
  if (cache_.size() == 0) {
    if (input.GetSize(0) < frame_sample_bytes_) {
      SPDLOG_TRACE(
          "{} bytes are not sufficient({}), temporarily store in cache.",
          input.GetSize(0), frame_sample_bytes_);
      cache_.insert(cache_.end(), (uint8_t*)input.GetData(0),
                    (uint8_t*)input.GetData(0) + input.GetSize(0));
      return kSuccess;
    } else if (input.GetSize(0) == frame_sample_bytes_) {
      // continue ...
    } else {
      // input.GetSize(0) > frame_sample_bytes_
      int remaining_bytes = input.GetSize(0) - frame_sample_bytes_;
      SPDLOG_TRACE("Too much data({}bytes), remaining {} bytes in cache.",
                   input.GetSize(0), remaining_bytes);
      cache_.insert(cache_.end(),
                    (uint8_t*)input.GetData(0) + frame_sample_bytes_,
                    (uint8_t*)input.GetData(0) + input.GetSize(0));
    }
  } else if (cache_.size() > frame_sample_bytes_) {
    erase_bytes = frame_sample_bytes_;
    SPDLOG_TRACE(
        "The cache already contains {} bytes, which is sufficiently full, "
        "adding another {} bytes. After use, {} bytes need to be removed.",
        cache_.size(), input.GetSize(0), erase_bytes);
    cache_.insert(cache_.end(), (uint8_t*)input.GetData(0),
                  (uint8_t*)input.GetData(0) + input.GetSize(0));
    input_buf = (char*)cache_.data();
  } else {
    // frame_sample_bytes_ > cache_.size() > 0
    cache_.insert(cache_.end(), (uint8_t*)input.GetData(0),
                  (uint8_t*)input.GetData(0) + input.GetSize(0));
    SPDLOG_TRACE("This way .... New cache {}bytes", cache_.size());
    if (cache_.size() >= frame_sample_bytes_) {
      input_buf = (char*)cache_.data();
      erase_bytes = frame_sample_bytes_;
      SPDLOG_TRACE(
          "The cache already contains {} bytes, which is sufficiently full. "
          "After use, {} bytes need to be removed.",
          cache_.size(), erase_bytes);
    } else {
      SPDLOG_TRACE(
          "{} bytes are not sufficient({}), temporarily store in cache.",
          cache_.size(), frame_sample_bytes_);
      return kSuccess;
    }
  }

  /* 1. 灌入数据开始编码 */
  int encoderSize =
      encoder_->OggopusEncode((const char*)input_buf, input_bytes);

  if (erase_bytes > 0) {
    cache_.erase(cache_.begin(), cache_.begin() + erase_bytes);
  }

  if (encoderSize != kSuccess) {
    SPDLOG_ERROR("[ENC:{}] OggopusEncode failed, ret {}",
                 static_cast<void*>(encoder_),
                 encoderSize);
    return kOggOpusEncodeFailed;
  }

  /* 2. 取出编码后数据 */
  encoderSize = 0;
  int data_len = encoder_->OggopusGetOuputSize();
  if (data_len > 0) {
    data_len =
        (data_len > output.GetCapacity(0)) ? output.GetCapacity(0) : data_len;
    encoderSize =
        encoder_->OggopusGetOuput((unsigned char*)output.GetData(0), data_len);
    if (encoderSize > 0) {
      output.SetSize(0, encoderSize);
#ifdef MP_INCLUDE_PROFILE
      if (this->debug_) {
        this->profile_.StopElementTiming(this->name_.c_str());
      }
#endif
    }
  }

  return kSuccess;
}

MediaStatus PcmToOpus::Reset() {
  std::unique_lock<decltype(this->encoder_lock_)> auto_lock(
      this->encoder_lock_);
  SPDLOG_DEBUG("Softrestart OggOpusEncoder ...");
  if (!encoder_) {
    SPDLOG_WARN("audio_encoder is inexistent in AudioEncoderSoftRestart.");
    return kEncoderInexistent;
  }

  SPDLOG_DEBUG("remainder opus data %dbytes in AudioEncoder, will reset.",
               encoder_->OggopusGetOuputSize());
  encoder_->OggopusSoftRestart();
  cache_.clear();
  return kSuccess;
}

MediaStatus PcmToOpus::create_locked() {
  if (this->encoder_) {
    encoder_->OggopusDestroy();
    delete encoder_;
  }

  this->encoder_ = new OggOpusEncoder();

  if (this->frame_size_ > 0) {
    SPDLOG_DEBUG("Set frame size {}", this->frame_size_);
    this->encoder_->SetFrameSize(this->frame_size_);
  }
  if (!this->bitrate_.empty()) {
    SPDLOG_DEBUG("Set bitrate {}", this->bitrate_);
    this->encoder_->SetBitrate(std::stoi(this->bitrate_));
  }
  if (!this->complexity_.empty()) {
    SPDLOG_DEBUG("Set complexity {}", this->complexity_);
    this->encoder_->SetComplexity(std::stoi(this->complexity_));
  }
  SPDLOG_DEBUG("Set VBR {}", this->vbr_);
  this->encoder_->SetVBRMode(this->vbr_);
  // this->encoder_->SetVBRConstraintMode(enable_constraint_vbr_);
  if (!this->application_.empty()) {
    std::string app_str = this->application_;
    std::transform(app_str.begin(), app_str.end(), app_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    SPDLOG_DEBUG("Set application type {}", app_str);
    this->encoder_->SetApplicationType(app_str);
  }
  if (!this->user_comment_.empty()) {
    SPDLOG_DEBUG("Set user comment {}", this->user_comment_);
    this->encoder_->AddUserComment(this->user_comment_);
  }

  // SetSampleRate will reset frame_sample_num/frame_sample_bytes
  int cur_sr = this->encoder_->GetSampleRate();
  if (cur_sr != this->sample_rate_) {
    this->encoder_->SetSampleRate(this->sample_rate_);
  }

  int cur_ch = this->encoder_->GetChannelNum();
  if (cur_ch != this->channels_) {
    this->encoder_->SetChannelNum(this->channels_);
  }

  SPDLOG_DEBUG("Set Debug mode {}", this->debug_ ? "true" : "false");
  this->encoder_->SetDebugMode(this->debug_);

  int ret = this->encoder_->OggopusEncoderCreate(
      reinterpret_cast<void*>(OggopusEncodedData), this, this->sample_rate_, this->channels_);
  if (ret == kSuccess) {
    SPDLOG_DEBUG(
        "OggopusEncoderCreate for OGGOPUS mode success, sample_rate({}), "
        "channels({}).",
        this->sample_rate_, this->channels_);
    this->frame_sample_bytes_ = this->encoder_->GetFrameSampleBytes();
  } else {
    SPDLOG_ERROR("OggopusEncoderCreate failed, errorcode:{}", ret);
  }

  return (MediaStatus)ret;
}

MediaStatus PcmToOpus::destroy_locked() {
  if (this->encoder_) {
    this->encoder_->OggopusDestroy();
    delete this->encoder_;
    this->encoder_ = nullptr;
  }
  SPDLOG_DEBUG("OggopusDestroy for OGGOPUS success.");
  return kSuccess;
}

}  // namespace mproc
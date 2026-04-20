/*
 * Copyright (c) 2026 season(season4675@gmail.com). All rights reserved.
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <time.h>

#include <mutex>
#include <string>

#include "oggopus_encoder_para.h"
#include "spdlog/spdlog.h"

namespace mproc {

#define OPUS_PACKAGE_NAME "opus-tools of Media-Processor"
#define OPUS_PACKAGE_VERSION "1.3.5"

class OggOpusEncoder {
 public:
  OggOpusEncoder();
  ~OggOpusEncoder();

  /**
   * @brief 初始化OggOpus编码器
   * @param sample_rate 采样率, 目前已经验证8K,16K,48K
   * @param channels 采样率, 目前支持单通道,双通道
   * @return
   */
  int Create(uint32_t sample_rate = 16000, uint32_t channels = 1);
  int Destroy();
  int Encode(const int16_t* pcm_data, size_t frame_size, uint8_t* encoded_buf,
             size_t encoded_buf_cap, size_t& encoded_size, bool is_eof = false);
  int Encode24(const int32_t* pcm_data, size_t frame_size, uint8_t* encoded_buf,
               size_t encoded_buf_cap, size_t& encoded_size,
               bool is_eof = false);
  int EncodeFloat(const float* pcm_data, size_t frame_size,
                  uint8_t* encoded_buf, size_t encoded_buf_cap,
                  size_t& encoded_size, bool is_eof = false);
  int Flush(uint8_t* encoded_buf, size_t encoded_buf_cap, size_t& encoded_size,
            bool is_eof = false, int depth = 16);
  int SoftReset();

  void SetSampleRate(int sample_rate);
  int GetSampleRate() const { return sample_rate_; }

  /**
   * @brief 设置比特率, 500 - 512000
   */
  void SetBitrate(int bitrate) {
    if (debug_) {
      spdlog::debug("SetBitrate {}", bitrate);
    }
    enc_bitrate_ = bitrate;
  }
  int GetBitrate() const { return enc_bitrate_; }

  void SetComplexity(int complexity) { enc_complexity_ = complexity; }
  int GetComplexity() const { return enc_complexity_; }

  void SetChannelNum(int ch);
  int GetChannelNum() { return channels_; }

  /**
   * @brief 启用/关闭编码器的 variable bitrate (VBR可变比特率)
   */
  void SetVBRMode(bool enable) { enable_vbr_ = enable; }
  bool GetVBRMode() const { return enable_vbr_; }
  /**
   * @brief VBR模式下, 限制编码器不能超过某个最大比特率
   */
  void SetVBRConstraintMode(bool enable) { enable_constraint_vbr_ = enable; }
  bool GetVBRConstraintMode() const { return enable_constraint_vbr_; }

  /**
   * @brief APPLICATION模式设置
   *        OPUS_APPLICATION_VOIP : "VOPI"
   *        OPUS_APPLICATION_AUDIO : "AUDIO"
   */
  void SetApplicationType(std::string type) { app_type_ = type; }
  std::string GetApplicationType() const { return app_type_; }

  /**
   * @brief 在封装中加入用户自定义内容
   */
  void AddUserComment(std::string comment) { user_comment_ = comment; }

  /**
   * @brief 设置编码的每帧字节长度, 即以FrameSampleBytes大小数据进行编码
   * @param bytes 最大为对应120ms音频字节数,
   * 验证可设置的对应音频长度为10ms,20ms,40ms,60ms,100ms,120ms.
   * @return 成功返回0，失败返回负值
   */
  int SetFrameSampleBytes(uint32_t bytes);
  uint32_t GetFrameSampleBytes() const { return frame_sample_bytes_; }

  /**
   * @brief 设置编码的每帧对应的音频时长
   * @param ms 音频时长为10ms,20ms,40ms,60ms,100ms,120ms.
   * @return 成功返回0，失败返回负值
   */
  int SetFrameSize(uint32_t ms);
  uint32_t GetFrameSize() const { return frame_duration_ms_; }

  void SetDebugMode(bool enable);

  // ===== 兼容旧版 API 接口 =====
  // 这些接口用于兼容 pcm_to_opus.cpp 的调用
  
  /**
   * @brief 旧版创建接口（带回调函数）
   */
  int OggopusEncoderCreate(void* callback, void* user_data, 
                           uint32_t sample_rate = 16000, 
                           uint32_t channels = 1);
  
  /**
   * @brief 旧版编码接口（输入 PCM 字节数据）
   */
  int OggopusEncode(const char* pcm_data, int data_len);
  
  /**
   * @brief 获取编码后数据大小
   */
  int OggopusGetOuputSize();
  
  /**
   * @brief 获取编码后数据
   */
  int OggopusGetOuput(unsigned char* output_buf, int buf_size);
  
  /**
   * @brief 推入编码后的数据到内部缓冲区
   */
  int OggopusPushEncodedData(const uint8_t* encoded_data, int data_len);
  
  /**
   * @brief 软重置编码器
   */
  int OggopusSoftRestart();
  
  /**
   * @brief 销毁编码器
   */
  int OggopusDestroy();

 private:
  size_t RemainingFrames(int depth = 16);
  int EncodePcm16Packet(uint8_t* encoded_data, size_t encoded_buf_cap,
                   size_t& encoded_size, bool is_eof);
  int EncodePcm24Packet(uint8_t* encoded_data, size_t encoded_buf_cap,
                     size_t& encoded_size, bool is_eof);
  int EncodePcm24PacketDirectly(const int32_t* pcm_data, size_t frame_size,
                             uint8_t* encoded_buf, size_t encoded_buf_cap,
                             size_t& encoded_size, bool is_eof);
  int EncodePcm32Packet(uint8_t* encoded_data, size_t encoded_buf_cap,
                     size_t& encoded_size, bool is_eof);
  int EncodePcm32PacketDirectly(const float* pcm_data, size_t frame_size,
                             uint8_t* encoded_buf, size_t encoded_buf_cap,
                             size_t& encoded_size, bool is_eof);
  int EncodeInner(const void* pcm_data, size_t samples,
                       uint8_t* encoded_data, size_t encoded_buf_cap,
                       size_t& encoded_size, bool is_eof, int depth = 16);

  void ResetParameters();

 private:
  std::mutex lock_;
  OggOpusEncoderPara* ogg_opus_para_;
  bool is_first_frame_processed_;
  size_t channels_;
  size_t sample_rate_;
  size_t sample_bits_;
  size_t frame_duration_ms_;  /* 一帧对应的时长, 单位毫秒 */
  size_t frame_sample_num_;   /* 每个通道的一帧的样本数 */
  size_t frame_sample_bytes_; /* 一帧的字节数 */
  size_t enc_bitrate_;
  uint32_t enc_complexity_;
  bool enable_vbr_;
  bool enable_constraint_vbr_;
  std::string app_type_;
  std::string user_comment_;
  bool debug_;
  
  // 兼容旧版 API 的内部缓冲区
  std::vector<uint8_t> encoded_data_buffer_;  // 存储编码后的数据
  void* encoded_callback_;                     // 编码数据回调函数
  void* callback_user_data_;                   // 回调用户数据
};

}  // namespace mproc

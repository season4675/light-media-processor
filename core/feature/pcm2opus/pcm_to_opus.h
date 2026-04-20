/*
 * Copyright (c) 2026 season(season4675@gmail.com). All rights reserved.
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <mutex>
#include <string>

#include "feature_processor.h"
#include "oggopus_encoder.h"
#ifdef MP_INCLUDE_PROFILE
#include "profile_utils.h"
#endif

namespace mproc {

class PcmToOpus : public IFeatureProcessor {
 public:
  PcmToOpus(MediaContext* ctx_handler);
  ~PcmToOpus();

  MediaStatus Create(MediaContext& context);
  MediaStatus Destroy();
  MediaStatus Process(MediaPacket& input, MediaConfig& src_frame,
                      MediaPacket& output, MediaConfig& dst_frame,
                      bool& update) override;
  MediaStatus Reset();

 private:
  MediaStatus CreateLocked();
  MediaStatus DestroyLocked();

  MediaContext* ctx_handler_;
  OggOpusEncoder* encoder_;
  std::vector<uint8_t> cache_;  // 保留用于兼容
  std::mutex encoder_lock_;
  int channels_;
  int sample_rate_;
  int frame_size_; /* ms */
  int frame_sample_bytes_;
  std::string bitrate_;
  std::string complexity_;
  bool vbr_;
  std::string application_;
  std::string user_comment_;
  bool debug_;
#ifdef MP_INCLUDE_PROFILE
  ProfileUtils profile_;
#endif
};

}  // namespace mproc
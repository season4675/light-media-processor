/*
 * Copyright (c) 2026 season(season4675@gmail.com). All rights reserved.
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <string>

#include "media_context.h"
#include "media_packet.h"

namespace mproc {

class IFeatureProcessor {
 public:
  virtual ~IFeatureProcessor() = default;
  virtual MediaStatus Create(MediaContext& context) = 0;
  virtual MediaStatus Destroy() = 0;
  virtual MediaStatus Process(MediaPacket& input, MediaConfig& src_frame,
                              MediaPacket& output, MediaConfig& dst_frame,
                              bool& update) = 0;
  virtual MediaStatus Reset() = 0;
  virtual std::string GetName();

  std::string name_;
};

}  // namespace funaudio
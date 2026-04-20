/*
 * Copyright (c) 2026 season(season4675@gmail.com). All rights reserved.
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "media_code.h"

namespace funaudio {

enum OggOpusHeadStatus {
  kOggOpusInsufficientData,
  kOggOpusInvalidInput,
  kOggOpusHeaderNotFound,
  kOggOpusHeadFound,
  kOggOpusTagFound,
};

class OggOpusUtils {
 public:
  OggOpusUtils();
  ~OggOpusUtils();

  int CheckOpusHeadOrTags(uint8_t *src, size_t len);
  OggOpusHeadStatus GetOggOpusInfo(uint8_t *input, size_t bytes, int &ch,
                                   int &sr, std::string &comment);

 private:
  std::vector<uint8_t> total_header_data_;
  const uint32_t max_header_bytes_ = 3072;
};

}  // namespace funaudio
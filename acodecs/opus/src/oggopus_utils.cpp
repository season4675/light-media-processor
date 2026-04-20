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

#include "oggopus_utils.h"
#include <cstdint>
#include <string>
#include "oggopus_data_struct.h"
#include "spdlog/spdlog.h"
#include "text_utils.h"

namespace funaudio {

OggOpusUtils::OggOpusUtils() {}
OggOpusUtils::~OggOpusUtils() {}

int OggOpusUtils::CheckOpusHeadOrTags(uint8_t* src, size_t len) {
  if (len < 8) return -1;
  if (memcmp(src, "OpusHead", 8) == 0) {
    return 1;
  }
  if (memcmp(src, "OpusTags", 8) == 0) {
    return 2;
  }
  return 0;
}

OggOpusHeadStatus OggOpusUtils::GetOggOpusInfo(uint8_t* input, size_t bytes,
                                               int& ch, int& sr,
                                               std::string& comment) {
  if (input == nullptr || bytes == 0) {
    return kOggOpusInvalidInput;
  }

  if (total_header_data_.size() > max_header_bytes_) {
    SPDLOG_ERROR("Too many data in GetOggOpusInfo");
    return kOggOpusHeaderNotFound;
  }

  total_header_data_.insert(total_header_data_.end(), input, input + bytes);

  OggDecodePageHeader ogg_header; /* sizeof(OggDecodePageHeader) == 27 */
  uint32_t cur_header_bytes = 0;
  uint32_t total_offset = 0;
  uint8_t* cur_src = total_header_data_.data() + total_offset;
  uint32_t new_offset = 0;
  bool first_frame_flag = false;

  ch = 0;
  sr = 0;
  comment = "";
  OggOpusHeadStatus status = kOggOpusInsufficientData;

  do {
    memcpy(&ogg_header, cur_src, sizeof(OggDecodePageHeader));

    char Oggs[5] = {0};
    memcpy(Oggs, ogg_header.Oggs, 4);
    if (!TextUtils::CheckCapturePattern(Oggs, 4, "OggS")) {
      SPDLOG_ERROR("get invalid ogg header ({}), current offset is {}!",
                   std::string(Oggs), total_offset);
      return status;
    }

    cur_header_bytes = sizeof(OggDecodePageHeader) + ogg_header.seg_num;
    total_offset += sizeof(OggDecodePageHeader);
    cur_src = total_header_data_.data() + total_offset;

    SPDLOG_TRACE(
        "get header_type:0x{:x} seg_num {} header_bytes:{} body_bytes:{}/{} in "
        "this ogg page.",
        ogg_header.header_type_flag, ogg_header.seg_num, cur_header_bytes,
        total_header_data_.size() - cur_header_bytes,
        total_header_data_.size());

    if (ogg_header.header_type_flag == 0x2) {
      first_frame_flag = true;
    }

    uint8_t* cur_seg_table = cur_src;
    total_offset += ogg_header.seg_num;
    cur_src = total_header_data_.data() + total_offset;

    for (int i = 0; i < ogg_header.seg_num; i++) {
      uint8_t cur_seg_table_bytes = *(cur_seg_table + i);
      SPDLOG_TRACE("  idx:{} -> {}bytes", i, cur_seg_table_bytes);
      if (cur_seg_table_bytes > 0) {
        if (first_frame_flag) {
          int ret = CheckOpusHeadOrTags(
              cur_src, total_header_data_.size() - total_offset);
          if (ret == 1) {
            unsigned char* tag_str[9] = {0};
            memcpy(tag_str, cur_src, 8);

            uint8_t channel_count = 0;
            memcpy(&channel_count, cur_src + 9, 1);

            uint32_t sample_rate = 0;
            memcpy(&sample_rate, cur_src + 12, 4);

            SPDLOG_TRACE("    tag pack {:s}, channel {} and sample_rate {}",
                         reinterpret_cast<const char*>(tag_str), channel_count,
                         sample_rate);

            if (channel_count != ch) {
              ch = channel_count;
            }
            if (sample_rate != sr) {
              sr = sample_rate;
            }
            status = kOggOpusHeadFound;
          } else if (ret == 2) {
            unsigned char* tag_str[9] = {0};
            memcpy(tag_str, cur_src, 8);

            uint32_t tag_offset = 8;
            uint32_t vendor_len = 0;
            memcpy(&vendor_len, cur_src + tag_offset, 4);
            tag_offset += 4;

            uint32_t comment_cout = 0;
            tag_offset += vendor_len;
            memcpy(&comment_cout, cur_src + tag_offset, 4);
            tag_offset += 4;

            SPDLOG_TRACE(
                "    tag pack {:s}, vendor length {} and comment count {}",
                reinterpret_cast<const char*>(tag_str), vendor_len,
                comment_cout);

            for (int i = 0; i < comment_cout; i++) {
              uint32_t comment_size = 0;
              memcpy(&comment_size, cur_src + tag_offset, 4);
              tag_offset += 4;

              if (comment_size > 0) {
                std::string comment(
                    reinterpret_cast<const char*>(cur_src + tag_offset),
                    comment_size);
                tag_offset += comment_size;
                SPDLOG_TRACE("User {}/{} comment {}", i, tag_offset, comment);
              }
            }

            status = kOggOpusTagFound;
            break;
          }
        }
        total_offset += cur_seg_table_bytes;
        cur_src = total_header_data_.data() + total_offset;
      }
    }  // for

    if (status == kOggOpusTagFound) break;
  } while (total_offset < total_header_data_.size());

  return status;
}

}  // namespace funaudio
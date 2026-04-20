
/*
 * Copyright (c) 2026 season(season4675@gmail.com). All rights reserved.
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */


#include "spdlog/spdlog.h"

namespace mproc {

// unsigned int OggOpusEncoderPara::ToUInt(unsigned char num[4], int len) {
//   unsigned int ret = 0;
//   if (len == 4) {
//     int i = 0;
//     for (i = 0; i < len; i++) {
//       ret |= ((unsigned int)num[i] << (i * 8));
//     }
//   }
//   return ret;
// }

// unsigned long long OggOpusEncoderPara::ToULL(unsigned char num[8],
//                                                  int len) {
//   unsigned long long ret = 0;
//   if (len == 8) {
//     int i = 0;
//     for (i = 0; i < len; i++) {
//       ret |= ((unsigned long long)num[i] << (i * 8));
//     }
//   }
//   return ret;
// }

void OggOpusEncoderPara::PrintOggPageHeader(const uint8_t *header,
                                                const int header_len) {
  OggDecodePageHeader opus_page_header;
  memcpy(&opus_page_header, header, header_len);
  SPDLOG_TRACE("Page num: {:03u}",
               ToUInt(opus_page_header.page_sequence_number, 4));
  SPDLOG_TRACE("  Ogg capture pattern: {:c} {:c} {:c} {:c}",
               opus_page_header.Oggs[0], opus_page_header.Oggs[1],
               opus_page_header.Oggs[2], opus_page_header.Oggs[3]);
  SPDLOG_TRACE(
      "  Type: {}(0:Continue, 2:First, 4:Last), granule_position: {:08llu}",
      opus_page_header.header_type_flag,
      ToULL(opus_page_header.granule_position, 8));
  SPDLOG_TRACE("  Seg_num: {}", opus_page_header.seg_num);
  for (int i = 0; i < opus_page_header.seg_num; i++) {
    SPDLOG_TRACE("  Seg_table: {}", opus_page_header.segment_table()[i]);
  }
}

int OggOpusEncoderPara::InitComment() {
  int opus_version_len = strlen(opus_version);
  int len = 8 + 4 + opus_version_len + 4;  // 37
  char *p = reinterpret_cast<char *>(malloc(len));
  if (nullptr == p) {
    SPDLOG_ERROR("malloc failed in CommentInit()");
    return kMemAllocError;
  } else {
    memset(p, 0, len);
  }

  /*
    Comment Header
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |      'O'      |      'p'      |      'u'      |      's'      |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |      'T'      |      'a'      |      'g'      |      's'      |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                     Vendor String Length                      |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      :                        Vendor String...                       :
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                   User Comment List Length                    |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                 User Comment #0 String Length                 |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      :                   User Comment #0 String...                   :
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                 User Comment #1 String Length                 |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      :                                                               :
  */
  /*
   * opus_version: libopus unknown-fixed
   * opus_version_len: 21
   * len: 37
   */
  // "OpusTags" 写入头8个字节
  memcpy(p, "OpusTags", 8);
  // 从第8个字节开始写入int 4个字节
  writeint(p, 8, opus_version_len);
  // 从第12个字节开始写入opus_version的21个字节
  memcpy(p + 12, opus_version, opus_version_len);
  // 从第(12+21)个字节开始写入4个字节
  writeint(p, 12 + opus_version_len, 0);
  ogg_encode_opt.comments_length = len;
  ogg_encode_opt.comments = p;

  return kSuccess;
}

int OggOpusEncoderPara::AddComment() {
  /*
    Comment Header
          0                   1                   2                   3
          0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     4b |      'O'      |      'p'      |      'u'      |      's'      | 4
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |      'T'      |      'a'      |      'g'      |      's'      | 8
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                  opus_version_len(int:4bytes)                 | 12
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |       opus_version_len(21bytes) libopus unknown-fixed  .....  | 16
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     5b |                             .....                             | 20-32
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     6b |    .......    |user_comment_list_length(int:4bytes)(后改为1)...| 36
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |    .......    |   tag_len+val_len=7+47(int:4bytes)  ......    | 40
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |    .......    |     tag("ENCODE=")   ....................     | 44
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                             .......                           | 48
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     7b |val("opusenc from opus-tools of Alibaba TongYi 1.3.5")47bytes..| 52
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                             .......                           | 56-92
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |               .......                         |               | 96
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        :                                                               :
  */
  const char *tag = "ENCODER";
  // InitComment后ogg_encode_opt.comments存了37个字节
  char *p = ogg_encode_opt.comments;
  // 从第8字节开始读取这段数据长度，即 vendor_length=21
  int vendor_length = readint(p, 8);
  // 从第33字节开始读取这段数据长度，即 user_comment_list_length=0
  int user_comment_list_length = readint(p, 8 + 4 + vendor_length);
  // tag_len = 7
  int tag_len = strlen(tag);
  // ENCODER_string: opusenc from opus-tools of Alibaba TongYi 1.3.5
  // val_len = 47
  int val_len = strlen(ENCODER_string);
  // len = 37 + 4 + 7 + 47
  int len = ogg_encode_opt.comments_length + 4 + tag_len + val_len;

  char *tmp = reinterpret_cast<char *>(realloc(p, len));
  if (tmp == NULL) {
    SPDLOG_ERROR("realloc failed in CommentAdd()");
    free(p);
    return -(kMemAllocError);
  } else {
    p = tmp;
  }
  /* length of comment */
  // 从第37字节开始写入tag和val(7+47)
  writeint(p, ogg_encode_opt.comments_length, tag_len + val_len);
  /* comment tag */
  // 从第(37+4)字节写入tag - "ENCODER"
  memcpy(p + ogg_encode_opt.comments_length + 4, tag, tag_len);
  // ENCODER的R替换成=
  (p + ogg_encode_opt.comments_length + 4)[tag_len - 1] = '='; /* separator */
  /* comment */
  // 从第(37+4+7)字节写入ENCODER_string(47)
  memcpy(p + ogg_encode_opt.comments_length + 4 + tag_len, ENCODER_string,
         val_len);
  // 从第(8+4+21)字节写入1
  writeint(p, 8 + 4 + vendor_length, user_comment_list_length + 1);
  ogg_encode_opt.comments_length = len;  // 95
  ogg_encode_opt.comments = p;

  return kSuccess;
}

int OggOpusEncoderPara::AddUserComment(std::string comment) {
  /*
    Comment Header
          0                   1                   2                   3
          0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     4b |      'O'      |      'p'      |      'u'      |      's'      | 4
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |      'T'      |      'a'      |      'g'      |      's'      | 8
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                  opus_version_len(int:4bytes)                 | 12
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |       opus_version_len(21bytes) libopus unknown-fixed  .....  | 16
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     5b |                             .....                             | 20-32
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     6b |    .......    |user_comment_list_length(int:4bytes)(后改为1)...| 36
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |    .......    |   tag_len+val_len=7+47(int:4bytes)  ......    | 40
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |    .......    |     tag("ENCODE=")   ....................     | 44
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                             .......                           | 48
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     7b |val("opusenc from opus-tools of Alibaba TongYi 1.3.5")47bytes..| 52
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                             .......                           | 56-92
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |               .......                         |new comment_len| 96
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |               .......                         |new comment .. | 100
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |               .......                                         |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        :                                                               :
  */

  SPDLOG_DEBUG("Add user comment: {}.", comment.c_str());

  // AddComment后ogg_encode_opt.comments存了95个字节
  char *p = ogg_encode_opt.comments;
  // 从第8字节开始读取这段数据长度，即 vendor_length=21
  int vendor_length = readint(p, 8);
  // 从第33字节开始读取这段数据长度，即 user_comment_list_length=1
  int user_comment_list_length = readint(p, 8 + 4 + vendor_length);
  // UserComment字节数
  int user_comment_len = comment.size();
  // len = 95 + 4 + user_comment_len
  int len = ogg_encode_opt.comments_length + 4 + user_comment_len;

  char *tmp = reinterpret_cast<char *>(realloc(p, len));
  if (tmp == NULL) {
    SPDLOG_ERROR("realloc failed in CommentAdd()");
    free(p);
    return -(kMemAllocError);
  } else {
    p = tmp;
  }
  /* length of comment */
  // 从第95字节开始写入user_comment_len
  writeint(p, ogg_encode_opt.comments_length, user_comment_len);
  /* comment */
  // 从第(95+4)字节写入user_comment
  memcpy(p + ogg_encode_opt.comments_length + 4, comment.c_str(),
         user_comment_len);
  // 从第(8+4+21)字节写入2
  writeint(p, 8 + 4 + vendor_length, user_comment_list_length + 1);
  ogg_encode_opt.comments_length = len;
  ogg_encode_opt.comments = p;

  return kSuccess;
}

/* 通过回调把数据推送到数据队列中 */
int OggOpusEncoderPara::WritePage() {
  int written = 0;
  written = ogg_encode_opt.callback_data_func(og.header, og.header_len,
                                              ogg_encode_opt.user_data);
  written += ogg_encode_opt.callback_data_func(og.body, og.body_len,
                                               ogg_encode_opt.user_data);

  if (debug) {
    PrintOggPageHeader(og.header, og.header_len);
    SPDLOG_TRACE("Write header {}bytes and body {}bytes.", og.header_len,
                 og.body_len);
  }
  return written;
}

};
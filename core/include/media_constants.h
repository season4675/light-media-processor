/*
 * Copyright (c) 2026 season(season4675@gmail.com). All rights reserved.
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#pragma once

namespace mproc {

enum MediaTaskType {
  kMediaConvertNone = 0,

  /* 音频转换处理器类型 */
  /* OggOpus解码器，输出为PCM */
  kMediaConvertOggOpusToPcm = 100,
  /* RawOpus解码器，输出为PCM */
  kMediaConvertRawOpusToPcm,
  /* OggOpus编码器，输入PCM */
  kMediaConvertPcmToOggOpus,
  /* RawOpus编码器，输入PCM */
  kMediaConvertPcmToRawOpus,

  /* 视频转换处理器类型 */
  /* H264解码器，输出为YUV420P */
  kMediaConvertH264ToYUV420P = 200,
  /* H264解码器，输出为JPEG */
  kMediaConvertH264ToJPEG,
  /* YUV420P编码器，输出为JPEG */
  kMediaConvertYUV420pToJPEG,

  /* 音频封装处理器类型 */
  /* 对裸opus数据进行Ogg封装 */
  kMediaConvertRawOpusToOggOpus = 300,
  /* 从OggOpus数据中抽出裸opus数据 */
  kMediaConvertOggOpusToRawOpus,
  /* 从WAV数据中抽出PCM数据 */
  kMediaConvertWavToPcm,

  /* 音视频滤镜类型，用于重采样、通道合并/拆分等等 */
  /* 音频PCM重采样 */
  kMediaConvertPcmResample = 400,
  /* YUV420P图片缩放 */
  kMediaConvertIyuvScale,
  /* reserved: 对音视频数据或文件进行解析，获得详细参数和信息 */
  kMediaConvertInfo = 500,

  kMediaConvertMax = 999,
};

enum MediaFormat {
  kMediaFmtNone,

  kMediaFmtAudio = 1, /* reserved */
  kMediaFmtPcm,
  kMediaFmtWav,
  kMediaFmtOggOpus,
  kMediaFmtRawOpus,

  kMediaFmtVideo = 100, /* reserved */
  kMediaFmtH264,
  kMediaFmtJPEG,
  kMediaFmtYUV420P,
  kMediaFmtRGB24, /* reserved */

  kMediaFmtMax = 999, /* reserved */
};

enum MediaLogLevel {
  MediaLogTrace,
  MediaLogDebug,
  MediaLogInfo,
  MediaLogWarn,
  MediaLogError,
  MediaLogCritical,
  MediaLogOff,
};

}  // namespace funaudio
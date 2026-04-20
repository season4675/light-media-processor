/*
 * Copyright (c) 2026 season(season4675@gmail.com). All rights reserved.
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#pragma once

namespace mproc {

enum MediaStatus {
  kSuccess = 0,
  kFlushAgain = 9,

  /* common 10 - 99 */
  kDefaultError = 10,
  kMemAllocError,
  kMemInsufficient,
  kJsonParseFailed,
  kUnsupportedMode,
  kInvalidInputParams,
  kEmptyProcessor,
  kProcessorCreateFailed,
  kInvalidContext,
  kInvalidWorkPipeline,

  /* avcodec */
  kEncoderExistent = 100,
  kDecoderExistent,
  kEncoderInexistent,
  kDecoderInexistent,
  kConvertInexistent,

  /* acodec - oggopus internal */
  kOpusEncoderCreateFailed = 150, /* Opus编码器创建失败 */
  kOpusDecoderCreateFailed,
  kOggOpusEncoderCreateFailed, /* OggOpus编码器创建失败 */
  kOggOpusInvalidState,        /* OggOpus状态不正确 */
  kOggOpusCreateFailed,        /* OggOpus创建失败 */
  kOggOpusStartFailed,         /* OggOpus启动失败 */
  kOggOpusEncodeFailed,        /* OggOpus编码失败 */
  kOggOpusDecodeFailed,        /* OggOpus解码失败 */
  kOggOpusStopFailed,          /* OggOpus停止失败 */
  kOggNoAvailablePage,         /* Ogg无有效页 */
  kOggNoAvailablePacket,
  kOggInitFailed,           /* Ogg组件初始化失败 */
  kOggAllocateMemoryFailed, /* Ogg同步缓存申请失败 */
  kOggBufferOverflow,       /* Ogg写入缓存失败 */
  kOggStreamPageinFailed,   /* Ogg页信息写入失败 */
  kOpusInvalidParameter,
  kOpusNoAvailablePage,

  /* filter - resample */
  kFilterResampleFailed = 500,
  kInsufficientWavData,

  /* vcodec */
  kH264DecodeFailed = 600,
  kH264DecoderCreateFailed,
  kJpegDecodeFailed,
  kIyuvScaleFormatError,
  kIyuvScaleFailed,
};

}
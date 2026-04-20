/*
 * Copyright (c) 2026 season(season4675@gmail.com). All rights reserved.
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <cstdio>
#include <cstring>

#include "ogg/ogg.h"

/* Opus Header
    Offset  Content
    0-7     Tag 'OpusHead'.
    8       Version, high 4 bit is major version, low 4 bit is minor version.
    9       Channels.
    10-11   Pre-skip.
    12-15   Sample rate.
    16-17   Gain.
    18      Channel mapping flag.
    [19-?]  Channel map.
*/
struct OpusHeader {
  int version;
  int channels; /* Number of channels: 1..255 */
  int preskip;
  ogg_uint32_t input_sample_rate;
  int gain; /* in dB S7.8 should be zero whenever possible */
  int channel_mapping;
  /* The rest is only used if channel_mapping != 0 */
  int nb_streams;
  int nb_coupled;
  unsigned char stream_map[255];
  OpusHeader()
      : version(0),
        channels(0),
        preskip(0),
        input_sample_rate(0),
        gain(0),
        channel_mapping(0),
        nb_streams(0),
        nb_coupled(0) {
    memset(stream_map, 0, sizeof(stream_map));
  }
};

int OpusHeaderToPacket(const OpusHeader *h, unsigned char *packet, int len);

extern const int wav_permute_matrix[8][8];

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

#pragma once

#include "ogg/ogg.h"
#include "oggopus_data_struct.h"
#include "opus_multistream.h"
#include "opus_types.h"

namespace mproc {

#ifdef __cplusplus
extern "C" {
#endif

void SetupPadder(OggEncodeOpt *ogg_encode_opt,
                 ogg_int64_t *original_sample_number);
void ClearPadder(OggEncodeOpt *ogg_encode_opt);

void RawOpen(OggEncodeOpt *ogg_encode_opt);
void WavClose(void *);

#ifdef __cplusplus
}
#endif

}  // namespace mproc

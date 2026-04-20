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

#include <stdlib.h>

#ifdef WIN32
#include <malloc.h>
#include <process.h>
#define getpid _getpid
#ifndef alloca
#define alloca _alloca
#endif

#else
#ifndef inline
#define inline __inline
#endif

#ifndef alloca
#define alloca _alloca
#endif
#endif

#define PACKAGE_NAME "opus-tools of Alibaba FunAudio"
#define PACKAGE_VERSION "1.3.2"
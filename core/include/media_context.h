/*
 * Copyright (c) 2026 season(season4675@gmail.com). All rights reserved.
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#pragma once

#include "media_config.h"
#include "media_constants.h"

namespace mproc {

class MediaContextImpl;
class MediaContext {
 public:
  MediaContext();

  /**
   * @brief Constructs a MediaContext configured for a specific media processing
   * task.
   *
   * Based on the given `type` (e.g., decoding, encoding, resampling), this
   * constructor pre-initializes the internal input and output MediaConfig
   * objects with appropriate formats, dimensions, or other task-specific
   * defaults.
   *
   * @param type The type of media processing task to be performed.
   */
  explicit MediaContext(MediaTaskType type);
  ~MediaContext();

  MediaContext& operator=(const MediaContext& other);
  MediaContext(const MediaContext& other);

  /**
   * @brief Returns a reference to the input MediaConfig.
   *
   * This frame describes the properties (format, size, sample rate, etc.) of
   * the expected input data for the media processing task.
   *
   * @return Reference to the input MediaConfig.
   */
  MediaConfig& Input();

  /**
   * @brief Returns a reference to the output MediaConfig.
   *
   * This frame describes the properties of the data that will be produced after
   * processing (e.g., decoded video resolution or encoded audio format).
   *
   * @return Reference to the output MediaConfig.
   */
  MediaConfig& Output();

  /**
   * @brief Sets the logging verbosity level for this context.
   *
   * Controls which log messages (e.g., debug, info, warn, error, none) are
   * emitted during media processing operations.
   *
   * @param level The desired log level.
   * @return MediaStatus indicating success or error (e.g., invalid level).
   */
  MediaStatus SetLogLevel(MediaLogLevel level);

  /**
   * @brief Returns a reference to the current log level.
   *
   * Allows direct inspection or modification of the logging level.
   *
   * @return Reference to the internal log level variable.
   */
  MediaLogLevel& LogLevel();

 private:
  MediaContextImpl* pimpl_;
  MediaLogLevel log_level_;
};

}  // namespace funaudio
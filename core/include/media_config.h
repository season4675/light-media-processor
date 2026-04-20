/*
 * Copyright (c) 2026 season(season4675@gmail.com). All rights reserved.
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#pragma once

#include "media_code.h"
#include "media_constants.h"

namespace mproc {

class MediaConfigImpl;
class MediaConfig {
 public:
  MediaConfig();
  ~MediaConfig();

  MediaConfig& operator=(const MediaConfig& other);

  /**
   * @brief Sets the media format (e.g., audio PCM, video YUV420P) for this
   * frame.
   *
   * This defines the fundamental data layout and type of the media content.
   *
   * @param fmt The media format to assign.
   * @return MediaStatus indicating success or error (e.g., unsupported format).
   */
  MediaStatus SetFormat(MediaFormat fmt);

  /**
   * @brief Retrieves the current media format of this frame.
   *
   * @return The media format (e.g., kMediaFmtPcm, kMediaFmtYUV420P).
   */
  MediaFormat GetFormat();

  /**
   * @brief Returns a human-readable string name for a given media format.
   *
   * If no argument is provided (or `fmt = -1`), returns the name of the current
   * frame's format. Otherwise, returns the name corresponding to the specified
   * `fmt`.
   *
   * @param fmt Media format enum value; use -1 to query the current frame's
   * format.
   * @return Null-terminated C-string representing the format name (e.g., "PCM",
   * "JPEG").
   */
  const char* GetFormatName(int fmt = -1);

  /**
   * @brief Sets the audio sample rate in Hz (e.g., 44100, 48000).
   *
   * Only applicable for audio frames. Ignored or returns error for video
   * frames.
   *
   * @param sr Sample rate in samples per second.
   * @return MediaStatus indicating success or error.
   */
  MediaStatus SetSampleRate(int sr);

  /**
   * @brief Gets the current audio sample rate in Hz.
   *
   * @return Sample rate (e.g., 48000); undefined or 0 if not an audio frame.
   */
  int GetSampleRate();

  /**
   * @brief Sets the number of audio channels (e.g., 1 for mono, 2 for stereo).
   *
   * Only applicable for audio frames.
   *
   * @param ch Number of channels.
   * @return MediaStatus indicating success or error.
   */
  MediaStatus SetChannels(int ch);

  /**
   * @brief Gets the number of audio channels.
   *
   * @return Channel count (e.g., 2); undefined or 0 if not an audio frame.
   */
  int GetChannels();

  /**
   * @brief Sets the width (in pixels) of the video frame.
   *
   * Only applicable for video frames.
   *
   * @param width Frame width in pixels.
   * @return MediaStatus indicating success or error.
   */
  MediaStatus SetWidth(int width);

  /**
   * @brief Gets the width (in pixels) of the video frame.
   *
   * @return Frame width; undefined or 0 if not a video frame.
   */
  int GetWidth();

  /**
   * @brief Sets the height (in pixels) of the video frame.
   *
   * Only applicable for video frames.
   *
   * @param height Frame height in pixels.
   * @return MediaStatus indicating success or error.
   */
  MediaStatus SetHeight(int height);

  /**
   * @brief Gets the height (in pixels) of the video frame.
   *
   * @return Frame height; undefined or 0 if not a video frame.
   */
  int GetHeight();

  /**
   * @brief Sets a custom string-valued option associated with this frame.
   *
   * These options allow passing additional metadata or control parameters
   * (e.g., codec-specific hints, timestamps, color space info).
   * See the project README or API documentation for supported keys and
   * semantics.
   *
   * @param key   Option name (null-terminated C-string).
   * @param value Option value (null-terminated C-string).
   * @return MediaStatus indicating success or error.
   */
  MediaStatus SetOptStr(const char* key, const char* value);

  /**
   * @brief Retrieves the value of a string-valued option.
   *
   * @param key          Option name.
   * @param default_val  Value to return if the key is not found (defaults to
   * empty string).
   * @return The stored string value, or `default_val` if not set.
   */
  const char* GetOptStr(const char* key, const char* default_val = "");

  /**
   * @brief Sets a custom integer-valued option.
   *
   * Used for numeric metadata (e.g., rotation angle, quality level).
   * Refer to documentation for valid keys.
   *
   * @param key   Option name.
   * @param value Integer value to store.
   * @return MediaStatus indicating success or error.
   */
  MediaStatus SetOptInt(const char* key, int value);

  /**
   * @brief Retrieves the value of an integer-valued option.
   *
   * @param key          Option name.
   * @param default_val  Value to return if the key is not found (defaults to
   * 0).
   * @return The stored integer, or `default_val` if not set.
   */
  int GetOptInt(const char* key, int default_val = 0);

  /**
   * @brief Sets a custom boolean-valued option.
   *
   * Useful for flags (e.g., "is_key_frame", "flip_horizontal").
   *
   * @param key   Option name.
   * @param value Boolean value to store.
   * @return MediaStatus indicating success or error.
   */
  MediaStatus SetOptBool(const char* key, bool value);

  /**
   * @brief Retrieves the value of a boolean-valued option.
   *
   * @param key          Option name.
   * @param default_val  Value to return if the key is not found (defaults to
   * false).
   * @return The stored boolean, or `default_val` if not set.
   */
  bool GetOptBool(const char* key, bool default_val = false);

 private:
  MediaConfigImpl* pimpl_;
};

}  // namespace funaudio
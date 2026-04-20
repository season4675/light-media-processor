/*
 * Copyright (c) 2026 season(season4675@gmail.com). All rights reserved.
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <cstddef>
#include <functional>

namespace mproc {

class MediaConfig;
class MediaPacketImpl;
class MediaPacket {
 public:
  MediaPacket();

  /**
   * @brief Constructs a MediaPacket from a MediaConfig.
   *
   * Initializes the packet with 3 internal buffers (e.g., Y, U, V planes for
   * video) based on the media format and dimensions in the given frame. The
   * predicted capacity for each buffer is computed from frame properties such
   * as resolution or sample count.
   *
   * @param frame Reference to the input MediaConfig used to determine layout
   * and size.
   */
  explicit MediaPacket(MediaConfig& frame);

  /**
   * @brief Constructs a MediaPacket from a MediaConfig and a source data size.
   *
   * Similar to the single-argument constructor, but uses the additional
   * `src_size` (e.g., compressed bitstream size) to refine buffer capacity
   * prediction. Primarily intended for initializing **output** MediaPackets
   * after decoding or processing, where the input size helps estimate the
   * required output buffer sizes.
   *
   * @param frame Reference to the input MediaConfig.
   * @param src_size Size (in bytes) of the source data (e.g., encoded packet
   * size).
   */
  explicit MediaPacket(MediaConfig& frame, size_t src_size);

  ~MediaPacket();

  MediaPacket(const MediaPacket& other);
  MediaPacket& operator=(const MediaPacket& other);
  MediaPacket& operator=(MediaPacket&& other) noexcept;

  /**
   * @brief Returns the number of memory buffers contained in this MediaPacket.
   * Default is 3.
   *
   * @return Number of buffers.
   */
  size_t BufferCount() const;

  /**
   * @brief Adds a new memory buffer to the MediaPacket.
   *
   * The caller is responsible for managing the lifetime of the provided memory.
   * This function does not allocate or copy data—it only stores the pointer and
   * metadata.
   *
   * @param data      Pointer to externally managed memory.
   * @param capacity  Total capacity of the buffer in bytes.
   * @param size      Valid data size in bytes (defaults to 0).
   * @return Index of the newly added buffer.
   */
  size_t AddBuffer(void* data, size_t capacity, size_t size = 0);

  /**
   * @brief Sets the properties of the buffer at the specified index.
   *
   * Behavior is undefined if `index` is out of bounds (>= BufferCount()).
   * Typically used when reusing a pre-allocated MediaPacket.
   *
   * @param data      Pointer to memory.
   * @param capacity  Total buffer capacity in bytes.
   * @param size      Valid data size in bytes (defaults to 0).
   * @param index     Buffer index (defaults to 0 for the first buffer).
   */
  void SetBuffer(void* data, size_t capacity, size_t size = 0, size_t index = 0,
                 std::function<void(void*)> deleter = nullptr);

  /**
   * @brief Retrieves the data pointer of the buffer at the given index.
   *
   * @param index Buffer index.
   * @return Pointer to the start of the data (castable to appropriate type,
   * e.g., uint8_t*).
   */
  void* GetData(size_t index);

  /**
   * @brief Sets the data pointer for the buffer at the specified index.
   *
   * Does not copy or manage the memory—only updates the internal pointer.
   * The caller must ensure the provided memory remains valid during use.
   *
   * @param index Buffer index.
   * @param data  New data pointer.
   */
  void SetData(size_t index, void* data,
               std::function<void(void*)> deleter = nullptr);

  /**
   * @brief Returns the total allocated capacity (in bytes) of the buffer at the
   * given index.
   *
   * @param index Buffer index.
   * @return Buffer capacity in bytes.
   */
  size_t GetCapacity(size_t index) const;

  /**
   * @brief Sets the total allocated capacity (in bytes) for the buffer at the
   * given index.
   *
   * @param index     Buffer index.
   * @param capacity  New capacity in bytes.
   */
  void SetCapacity(size_t index, size_t capacity);

  /**
   * @brief Returns the predicted (recommended) capacity for the buffer at the
   * given index.
   *
   * This value is computed during construction based on the MediaConfig's
   * properties (e.g., resolution, format) and is used as a hint for optimal
   * buffer allocation.
   *
   * @param index Buffer index.
   * @return Predicted capacity in bytes.
   */
  size_t GetPredictedCapacity(size_t index) const;

  /**
   * @brief Returns the size of valid data (in bytes) in the buffer at the given
   * index.
   *
   * For example, after decoding a 1920x1080 YUV frame, the Y plane size would
   * be 1920*1080.
   *
   * @param index Buffer index.
   * @return Valid data size in bytes.
   */
  size_t GetSize(size_t index) const;

  /**
   * @brief Sets the size of valid data (in bytes) for the buffer at the given
   * index.
   *
   * @param index Buffer index.
   * @param size  New valid data size in bytes.
   */
  void SetSize(size_t index, size_t size);

 private:
  MediaPacketImpl* pimpl_;
};

}  // namespace funaudio
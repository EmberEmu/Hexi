//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/pmc/buffer_write.h>
#include <hexi/shared.h>
#include <hexi/concepts.h>
#include <hexi/exception.h>
#include <ranges>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace hexi::pmc {

using namespace detail;

template<byte_oriented buf_type>
requires std::ranges::contiguous_range<buf_type>
class buffer_write_adaptor : public buffer_write {
	buf_type& buffer_;

public:
	buffer_write_adaptor(buf_type& buffer)
		: buffer_(buffer) {
		write_ = buffer.size();
	}

	buffer_write_adaptor(buf_type& buffer, init_empty_t)
		: buffer_(buffer) {}

	/**
	 * @brief Write data to the container.
	 * 
	 * @note The source buffer must not overlap with the underlying buffer
	 * being used by the buffer_adaptor.
	 * 
	 * @param source Pointer to the data to be written.
	 */
	void write(auto& source) {
		write(&source, sizeof(source));
	}

	/**
	 * @brief Write provided data to the container.
	 *
	 * @note The source buffer must not overlap with the underlying buffer
	 * being used by the buffer_adaptor.
	 * 
	 * @param source Pointer to the data to be written.
	 * @param length Number of bytes to write from the source.
	 */
	void write(const void* source, std::size_t length) override {
		assert(source && !region_overlap(source, length, buffer_.data(), buffer_.size()));
		const auto min_req_size = write_ + length;

		if(buffer_.size() < min_req_size) [[likely]] {
			if constexpr(has_resize_overwrite<buf_type>) {
				buffer_.resize_and_overwrite(min_req_size, [](char*, std::size_t size) {
					return size;
				});
			} else if constexpr(has_resize<buf_type>) {
				buffer_.resize(min_req_size);
			} else {
				HEXI_THROW(buffer_overflow(free(), length, write_));
			}
		}

		std::memcpy(buffer_.data() + write_, source, length);
		write_ += length;
	}

	/**
	 * @brief Reserves a number of bytes within the container for future use.
	 * 
	 * @note This is a non-binding request, meaning the buffer may not reserve
	 * any additional space, such as in the case where it is not supported.
	 *
	 * @param length The number of bytes that the container should reserve.
	 */
	void reserve(const std::size_t length) override {
		if constexpr(has_reserve<buf_type>) {
			buffer_.reserve(length);
		}
	}

	/**
	 * @brief Determines whether this container can write seek.
	 *
	 * @return Whether this container is capable of write seeking.
	 */
	bool can_write_seek() const override {
		return true;
	}

	/**
	 * @brief Performs write seeking within the container.
	 *
	 * @param direction Specify whether to seek in a given direction or to absolute seek.
	 * @param offset The offset relative to the seek direction or the absolute value
	 * when using absolute seeking.
	 */
	void write_seek(const buffer_seek direction, const std::size_t offset) override {
		switch(direction) {
			case buffer_seek::sk_backward:
				write_ -= offset;
				break;
			case buffer_seek::sk_forward:
				write_ += offset;
				break;
			case buffer_seek::sk_absolute:
				write_ = offset;
		}
	}

	/**
	 * @return Pointer to the underlying storage.
	 */
	auto storage() const {
		return buffer_.data();
	}

	/**
	 * @return Pointer to the underlying storage.
	 */
	auto storage() {
		return buffer_.data();
	}

	/**
	 * @return Pointer to the location within the buffer where the next write
	 * will be made.
	 */
	auto write_ptr() {
		return buffer_.data() + write_;
	}

	/**
	 * @return Pointer to the location within the buffer where the next write
	 * will be made.
	 */
	auto write_ptr() const {
		return buffer_.data() + write_;
	}

	/**
	* @return The current write offset.
	*/
	auto write_offset() const {
		return write_;
	}

	/**
	 * @brief Clear the underlying buffer and reset state.
	 */
	void clear() {
		write_ = 0;

		if constexpr(has_clear<buf_type>) {
			buffer_.clear();
		}
	}

	/**
	* @brief Advances the write cursor.
	* 
	* @param size The number of bytes by which to advance the write cursor.
	*/
	void advance_write(std::size_t bytes) {
		assert(buffer_.size() >= (write_ + bytes));
		write_ += bytes;
	}

	/**
	 * @brief The amount of free space.
	 * 
	 * @return The number of bytes of free space within the container.
	 */
	std::size_t free() const {
		return buffer_.size() - write_;
	}
};

} // pmc, hexi

//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/pmc/buffer_read.h>
#include <hexi/shared.h>
#include <hexi/concepts.h>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace hexi::pmc {

using namespace detail;

template<byte_oriented buf_type>
requires std::ranges::contiguous_range<buf_type>
class buffer_read_adaptor : public buffer_read {
	buf_type& buffer_;

public:
	buffer_read_adaptor(buf_type& buffer)
		: buffer_(buffer) {
		write_ = buffer_.size();
	}

	buffer_read_adaptor(buf_type& buffer, init_empty_t)
		: buffer_(buffer) {}

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @note The destination buffer must not overlap with the underlying buffer
	 * being used by the buffer_adaptor.
	 * 
	 * @tparam T The destination type.
	 * @param[out] destination The buffer to copy the data to.
	 */
	template<typename T>
	void read(T* destination) {
		read(destination, sizeof(T));
	}

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 *
	 * @note The destination buffer must not overlap with the underlying buffer
	 * being used by the buffer_adaptor.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to read into the buffer.
	 */
	void read(void* destination, std::size_t length) override {
		assert(destination && !region_overlap(buffer_.data(), buffer_.size(), destination, length));
		std::memcpy(destination, buffer_.data() + read_, length);
		read_ += length;
	}

	/**
	 * @brief Copies a number of bytes to the provided buffer but without advancing
	 * the read cursor.
	 * 
	 * @param[out] destination The buffer to copy the data to.
	 */
	template<typename T>
	void copy(T* destination) const {
		copy(destination, sizeof(T));
	}

	/**
	 * @brief Copies a number of bytes to the provided buffer but without advancing
	 * the read cursor.
	 * 
	 * @note The destination buffer must not overlap with the underlying buffer
	 * being used by the buffer_adaptor.
	 * 
	 * @param[out] destination The buffer to copy the data to.
	 * @param length The number of bytes to copy.
	 */
	void copy(void* destination, std::size_t length) const override {
		assert(destination && !region_overlap(buffer_.data(), buffer_.size(), destination, length));
		std::memcpy(destination, buffer_.data() + read_, length);
	}

	/**
	 * @brief Skip over a number of bytes.
	 *
	 * Skips over a number of bytes from the container. This should be used
	 * if the container holds data that you don't care about but don't want
	 * to have to read it to another buffer to access data beyond it.
	 * 
	 * @param length The number of bytes to skip.
	 */
	void skip(std::size_t length) override {
		read_ += length;
	}

	/**
	 * @brief Returns the size of the container.
	 * 
	 * @note The value returned may be higher than the total number of bytes
	 * that can be read from this stream, if a read limit was set during
	 * construction. Use read_max() to determine how many bytes can be
	 * read from this stream.
	 * 
	 * @return The number of bytes of data available to read within the stream.
	 */
	std::size_t size() const override {
		return write_ - read_;
	}

	/**
	 * @brief Whether the container is empty.
	 * 
	 * @return Returns true if the container is empty (has no data to be read).
	 */
	[[nodiscard]]
	bool empty() const override {
		return read_ == write_;
	}

	/**
	 * @brief Retrieves a reference to the specified index within the container.
	 * 
	 * @param index The index within the container.
	 * 
	 * @return A reference to the value at the specified index.
	 */
	const std::byte& operator[](const std::size_t index) const override {
		return reinterpret_cast<const std::byte*>(buffer_.data() + read_)[index];
	}

	/**
	 * @return Pointer to the data available for reading.
	 */
	auto read_ptr() const {
		return buffer_.data() + read_;
	}

	/**
	 * @return The current read offset.
	 */
	auto read_offset() const {
		return read_;
	}

	/**
	 * @brief Attempts to locate the provided value within the container.
	 * 
	 * @param value The value to locate.
	 * 
	 * @return The position of value or npos if not found.
	 */
	std::size_t find_first_of(std::byte val) const override {
		const auto data = read_ptr();

		for(std::size_t i = 0, j = size(); i < j; ++i) {
			if(static_cast<std::byte>(data[i]) == val) {
				return i;
			}
		}

		return npos;
	}

	/*
	 * @brief Resets both the read and write cursors back to the beginning
	 * of the buffer.
	 * 
	 * @note The underlying buffer will not be cleared but should be treated
	 * as thought it has been.
	 */
	void clear() {
		read_ = 0;

		if constexpr(has_clear<buf_type>) {
			buffer_.clear();
		}
	}
};

} // pmc, hexi

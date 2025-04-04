//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/shared.h>
#include <hexi/concepts.h>
#include <hexi/exception.h>
#include <ranges>
#include <type_traits>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace hexi {

using namespace detail;

template<byte_oriented buf_type, bool space_optimise = true>
requires std::ranges::contiguous_range<buf_type>
class buffer_adaptor final {
public:
	using value_type  = typename buf_type::value_type;
	using size_type   = typename buf_type::size_type;
	using offset_type = typename buf_type::size_type;
	using contiguous  = is_contiguous;
	using seeking     = supported;

	static constexpr auto npos { static_cast<size_type>(-1) };

private:
	buf_type& buffer_;
	size_type read_;
	size_type write_;

public:
	buffer_adaptor(buf_type& buffer)
		: buffer_(buffer),
		  read_(0),
		  write_(buffer.size()) {}

	buffer_adaptor(buf_type& buffer, init_empty_t)
		: buffer_(buffer),
		  read_(0),
		  write_(0) {}

	buffer_adaptor(buffer_adaptor&& rhs) = delete;
	buffer_adaptor& operator=(buffer_adaptor&&) = delete;
	buffer_adaptor& operator=(const buffer_adaptor&) = delete;
	buffer_adaptor(const buffer_adaptor&) = delete;

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 */
	template<typename T>
	void read(T* destination) {
		read(destination, sizeof(T));
	}

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to read into the buffer.
	 */
	void read(void* destination, size_type length) {
		assert(destination);
		copy(destination, length);
		read_ += length;

		if constexpr(space_optimise) {
			if(read_ == write_) {
				read_ = write_ = 0;
			}
		}
	}

	/**
	 * @brief Copies a number of bytes to the provided buffer but without advancing
	 * the read cursor.
	 * 
	 * @param destination The buffer to copy the data to.
	 */
	template<typename T>
	void copy(T* destination) const {
		copy(destination, sizeof(T));
	}

	/**
	 * @brief Copies a number of bytes to the provided buffer but without advancing
	 * the read cursor.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to copy.
	 */
	void copy(void* destination, size_type length) const {
		assert(destination && !region_overlap(buffer_.data(), buffer_.size(), destination, length));
		std::memcpy(destination, read_ptr(), length);
	}

	/**
	 * @brief Skip over length bytes.
	 * 
	 * Skips over a number of bytes from the container. This should be used
	 * if the container holds data that you don't care about but don't want
	 * to have to read it to another buffer to move beyond it.
	 * 
	 * @param length The number of bytes to skip.
	 */
	void skip(size_type length) {
		read_ += length;

		if constexpr(space_optimise) {
			if(read_ == write_) {
				read_ = write_ = 0;
			}
		}
	}

	/**
	 * @brief Write data to the container.
	 * 
	 * @param source Pointer to the data to be written.
	 */
	void write(const auto& source) {
		write(&source, sizeof(source));
	}

	/**
	 * @brief Write provided data to the container.
	 * 
	 * @param source Pointer to the data to be written.
	 * @param length Number of bytes to write from the source.
	 */
	void write(const void* source, size_type length) {
		assert(source && !region_overlap(source, length, buffer_.data(), buffer_.size()));
		const auto min_req_size = write_ + length;

		if(buffer_.size() < min_req_size) [[unlikely]] {
			if constexpr(has_resize_overwrite<buf_type>) {
				buffer_.resize_and_overwrite(min_req_size, [](char*, size_type size) {
					return size;
				});
			} else if constexpr(has_resize<buf_type>) {
				buffer_.resize(min_req_size);
			} else {
				HEXI_THROW(buffer_overflow(length, write_, free()));
			}
		}

		std::memcpy(write_ptr(), source, length);
		write_ += length;
	}

	/**
	 * @brief Attempts to locate the provided value within the container.
	 * 
	 * @param value The value to locate.
	 * 
	 * @return The position of value or npos if not found.
	 */
	size_type find_first_of(value_type val) const {
		const auto data = read_ptr();

		for(size_type i = 0, j = size(); i < j; ++i) {
			if(data[i] == val) {
				return i;
			}
		}

		return npos;
	}
	
	/**
	 * @brief Returns the size of the container.
	 * 
	 * @return The number of bytes of data available to read within the container.
	 */
	size_type size() const {
		return write_ - read_;
	}

	/**
	 * @brief Whether the container is empty.
	 * 
	 * @return Returns true if the container is empty (has no data to be read).
	 */
	[[nodiscard]]
	bool empty() const {
		return read_ == write_;
	}

	/**
	 * @brief Retrieves a reference to the specified index within the container.
	 * 
	 * @param index The index within the container.
	 * 
	 * @return A reference to the value at the specified index.
	 */
	value_type& operator[](const size_type index) {
		return read_ptr()[index];
	}

	/**
	 * @brief Retrieves a reference to the specified index within the container.
	 * 
	 * @param index The index within the container.
	 * 
	 * @return A reference to the value at the specified index.
	 */
	const value_type& operator[](const size_type index) const {
		return read_ptr()[index];
	}

	/**
	 * @brief Determine whether the adaptor supports write seeking.
	 * 
	 * This is determined at compile-time and does not need to checked at
	 * run-time.
	 * 
	 * @return True if write seeking is supported, otherwise false.
	 */
	constexpr static bool can_write_seek() {
		return std::is_same_v<seeking, supported>;
	}

	/**
	 * @brief Performs write seeking within the container.
	 * 
	 * @param direction Specify whether to seek in a given direction or to absolute seek.
	 * @param offset The offset relative to the seek direction or the absolute value
	 * when using absolute seeking.
	 */
	void write_seek(const buffer_seek direction, const offset_type offset) {
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
	 * @return Pointer to the data available for reading.
	 */
	auto read_ptr() const {
		return buffer_.data() + read_;
	}

	/**
	 * @return Pointer to the data available for reading.
	 */
	auto read_ptr() {
		return buffer_.data() + read_;
	}

	/**
	 * @return Pointer to the location within the buffer where the next write
	 * will be made.
	 */
	auto write_ptr() const  {
		return buffer_.data() + write_;
	}

	/**
	 * @return Pointer to the location within the buffer where the next write
	 * will be made.
	 */
	auto write_ptr() {
		return buffer_.data() + write_;
	}

	/**
	 * @return Pointer to the data available for reading.
	 */
	auto data() const {
		return buffer_.data() + read_;
	}

	/**
	 * @return Pointer to the data available for reading.
	 */
	auto data() {
		return buffer_.data() + read_;
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
	 * @brief Advances the write cursor.
	 * 
	 * @param size The number of bytes by which to advance the write cursor.
	 */
	void advance_write(size_type bytes) {
		assert(buffer_.size() >= (write_ + bytes));
		write_ += bytes;
	}

	auto free() const {
		return buffer_.size() - write_;
	}

	void clear() {
		read_ = write_ = 0;
	}
};

} // hexi

//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/exception.h>
#include <hexi/shared.h>
#include <hexi/concepts.h>
#include <array>
#include <span>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace hexi {

using namespace detail;

template<byte_type storage_type, std::size_t buf_size>
class static_buffer final {
	std::array<storage_type, buf_size> buffer_ = {};
	std::size_t read_ = 0;
	std::size_t write_ = 0;

public:
	using size_type       = typename decltype(buffer_)::size_type;
	using offset_type     = size_type;
	using value_type      = storage_type;
	using contiguous      = is_contiguous;
	using seeking         = supported;

	static constexpr auto npos { static_cast<size_type>(-1) };
	
	static_buffer() = default;

	template<typename... T> 
	static_buffer(T&&... vals) : buffer_{ std::forward<T>(vals)... } {
		write_ = sizeof... (vals);
	}

	static_buffer(static_buffer&& rhs) = default;
	static_buffer& operator=(static_buffer&&) = default;
	static_buffer& operator=(const static_buffer&) = default;
	static_buffer(const static_buffer&) = default;

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
		copy(destination, length);
		read_ += length;

		if(read_ == write_) {
			read_ = write_ = 0;
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
	 * @note The destination buffer address must not belong to the static_buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to copy.
	 */
	void copy(void* destination, size_type length) const {
		assert(!region_overlap(buffer_.data(), buffer_.size(), destination, length));

		if(length > size()) {
			HEXI_THROW(buffer_underrun(length, read_, size()));
		}

		std::memcpy(destination, read_ptr(), length);
	}

	/**
	 * @brief Attempts to locate the provided value within the container.
	 * 
	 * @param value The value to locate.
	 * 
	 * @return The position of value or npos if not found.
	 */
	size_type find_first_of(value_type val) const noexcept {
		const auto data = read_ptr();

		for(size_type i = 0, j = size(); i < j; ++i) {
			if(data[i] == val) {
				return i;
			}
		}

		return npos;
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
	void skip(const size_type length) {
		read_ += length;

		if(read_ == write_) {
			read_ = write_ = 0;
		}
	}

	/**
	 * @brief Advances the write cursor.
	 * 
	 * @param size The number of bytes by which to advance the write cursor.
	 */
	void advance_write(size_type bytes) {
		assert(free() >= bytes);
		write_ += bytes;
	}

	/**
	 * @brief Resizes the buffer.
	 * 
	 * @param size The new size of the buffer.
	 * 
	 */
	void resize(size_type size) {
		if(size > buffer_.size()) {
			throw exception("attempted to resize static_buffer to larger than capacity");
		}

		write_ = size;
	}

	/**
	 * @brief Clears the container.
	 */
	void clear() {
		read_ = write_ = 0;
	}

	/**
	 * @brief Moves any unread data to the front of the buffer, freeing space at the end.
	 * If a move is performed, pointers obtained from read/write_ptr() will be invalidated.
	 * 
	 * If the buffer contains no data available for reading, this function will have no effect.
	 * 
	 * @return True if additional space was made available.
	 */
	bool defragment() {
		if(read_ == 0) {
			return false;
		}

		write_ = size();
		std::memmove(buffer_.data(), read_ptr(), write_);
		read_ = 0;
		return true;
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
	 * @brief Whether the container is empty.
	 * 
	 * @return Returns true if the container is empty (has no data to be read).
	 */
	[[nodiscard]]
	bool empty() const {
		return write_ == read_;
	}

	/**
	 * @return Whether the container is full and cannot be further written to.
	 */
	bool full() const {
		return write_ == capacity();
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
	 * @note The source buffer address must not belong to the static_buffer.
	 * 
	 * @param source Pointer to the data to be written.
	 * @param length Number of bytes to write from the source.
	 */
	void write(const void* source, size_type length) {
		assert(!region_overlap(source, length, buffer_.data(), buffer_.size()));

		if(free() < length) {
			HEXI_THROW(buffer_overflow(length, write_, free()));
		}

		std::memcpy(write_ptr(), source, length);
		write_ += length;
	}

	/**
	 * @brief Performs write seeking within the container.
	 * 
	 * @param direction Specify whether to seek in a given direction or to absolute seek.
	 * @param offset The offset relative to the seek direction or the absolute value
	 * when using absolute seeking.
	 */
	void write_seek(const buffer_seek direction, const size_type offset) {
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
	 * @return An iterator to the beginning of data available for reading.
	 */
	auto begin() {
		return buffer_.begin() + read_;
	}

	/**
	 * @return An iterator to the beginning of data available for reading.
	 */
	auto begin() const {
		return buffer_.begin() + read_;
	}

	/**
	 * @return An iterator to the end of data available for reading.
	 */
	auto end() {
		return buffer_.begin() + write_;
	}

	/**
	 * @return An iterator to the end of data available for reading.
	 */
	auto end() const {
		return buffer_.begin() + write_;
	}

	/**
	 * @brief Overall capacity of the container.
	 * 
	 * @return The container's total size in bytes.
	 */
	constexpr static size_type capacity() {
		return buf_size;
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
	 * @brief The amount of free space.
	 * 
	 * @return The number of bytes of free space within the container.
	 */
	size_type free() const {
		return buf_size - write_;
	}

	/**
	 * @return Pointer to the data available for reading.
	 */
	const value_type* data() const {
		return read_ptr();
	}

	/**
	 * @return Pointer to the data available for reading.
	 */
	value_type* data() {
		return read_ptr();
	}

	/**
	 * @return Pointer to the data available for reading.
	 */
	const value_type* read_ptr() const {
		return buffer_.data() + read_;
	}

	/**
	 * @return Pointer to the data available for reading.
	 */
	value_type* read_ptr() {
		return buffer_.data() + read_;
	}

	/**
	 * @return Pointer to the location within the buffer where the next write
	 * will be made.
	 */
	const value_type* write_ptr() const {
		return buffer_.data() + write_;
	}

	/**
	 * @return Pointer to the location within the buffer where the next write
	 * will be made.
	 */
	value_type* write_ptr() {
		return buffer_.data() + write_;
	}

	/**
	 * @return Pointer to the underlying storage.
	 */
	value_type* storage() {
		return buffer_.data();
	}

	/**
	 * @return Pointer to the underlying storage.
	 */
	const value_type* storage() const {
		return buffer_.data();
	}

	/**
	 * @brief Retrieves a span representing the data available for reading contained within
	 * underlying storage.
	 * 
	 * @return A span over the data waiting to be read from the container.
	 */
	std::span<const value_type> read_span() const {
		return { read_ptr(), size() };
	}

	/**
	 * @brief Retrieves a span representing the free space within the underlying storage.
	 * 
	 * @return A span over the container's free space.
	 */
	std::span<value_type> write_span() {
		return { write_ptr(), free() };
	}
};

} // hexi

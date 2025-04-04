//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/pmc/stream_base.h>
#include <hexi/pmc/buffer_write.h>
#include <hexi/concepts.h>
#include <hexi/endian.h>
#include <hexi/shared.h>
#include <hexi/stream_adaptors.h>
#include <algorithm>
#include <string>
#include <string_view>
#include <type_traits>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace hexi::pmc {

using namespace detail;

class binary_stream_writer : virtual public stream_base {
	buffer_write& buffer_;
	std::size_t total_write_;

	inline void write(const void* data, const std::size_t size) {
		if(state() == stream_state::ok) [[likely]] {
			buffer_.write(data, size);
			total_write_ += size;
		}
	}

	template<typename container_type>
	void write_container(container_type& container) {
		if constexpr(pod<typename container_type::value_type> && std::ranges::contiguous_range<container_type>) {
			const auto bytes = container.size() * sizeof(typename container_type::value_type);
			write(container.data(), bytes);
		} else {
			for(auto& element : container) {
				*this << element;
			}
		}
	}
public:
	explicit binary_stream_writer(buffer_write& source)
		: stream_base(source),
		  buffer_(source),
		  total_write_(0) {}

	binary_stream_writer(binary_stream_writer&& rhs) noexcept
		: stream_base(rhs),
		  buffer_(rhs.buffer_), 
		  total_write_(rhs.total_write_) {
		rhs.total_write_ = static_cast<std::size_t>(-1);
		rhs.set_state(stream_state::invalid_stream);
	}

	binary_stream_writer& operator=(binary_stream_writer&&) = delete;
	binary_stream_writer& operator=(const binary_stream_writer&) = delete;
	binary_stream_writer(const binary_stream_writer&) = delete;

	void serialise(auto&& object) {
		stream_write_adaptor adaptor(*this);
		object.serialise(adaptor);
	}

	binary_stream_writer& operator<<(has_shl_override<binary_stream_writer> auto&& data) {
		return data.operator<<(*this);
	}

	template<typename T>
	requires has_serialise<T, stream_write_adaptor<binary_stream_writer>>
	binary_stream_writer& operator<<(T& data) {
		serialise(data);
		return *this;
	}

	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	binary_stream_writer& operator<<(endian_func adaptor) {
		const auto converted = adaptor.to();
		write(&converted, sizeof(converted));
		return *this;
	}

	template<pod T>
	requires (!has_shl_override<T, binary_stream_writer>)
	binary_stream_writer& operator<<(const T& data) {
		write(&data, sizeof(data));
		return *this;
	}

	template<typename T>
	binary_stream_writer& operator<<(prefixed<T> adaptor) {
		auto size = static_cast<std::uint32_t>(adaptor->size());
		endian::native_to_little_inplace(size);
		write(&size, sizeof(size));
		write(adaptor->data(), adaptor->size());
		return *this;
	}

	template<typename T>
	binary_stream_writer& operator<<(prefixed_varint<T> adaptor) {
		varint_encode(*this, adaptor->size());
		write(adaptor->data(), adaptor->size());
		return *this;
	}

	template<typename T>
	requires std::is_same_v<std::decay_t<T>, std::string_view>
	binary_stream_writer& operator<<(null_terminated<T> adaptor) {
		assert(adaptor->find_first_of('\0') == adaptor->npos);
		write(adaptor->data(), adaptor->size());
		const char terminator = '\0';
		write(&terminator, 1);
		return *this;
	}

	template<typename T>
	requires std::is_same_v<std::decay_t<T>, std::string>
	binary_stream_writer& operator<<(null_terminated<T> adaptor) {
		assert(adaptor->find_first_of('\0') == adaptor->npos);
		write(adaptor->data(), adaptor->size() + 1); // yes, the standard allows this
		return *this;
	}

	template<typename T>
	binary_stream_writer& operator<<(raw<T> adaptor) {
		write(adaptor->data(), adaptor->size());
		return *this;
	}

	binary_stream_writer& operator<<(std::string_view string) {
		return (*this << prefixed(string));
	}

	binary_stream_writer& operator<<(const std::string& string) {
		return (*this << prefixed(string));
	}

	binary_stream_writer& operator<<(const char* data) {
		assert(data);
		const auto len = std::strlen(data);
		write(data, len + 1); // include terminator
		return *this;
	}

	template<std::ranges::contiguous_range range>
	requires pod<typename range::value_type>
	binary_stream_writer& operator <<(const range& data) {
		const auto write_size = data.size() * sizeof(typename range::value_type);
		write(data.data(), write_size);
		return *this;
	}

	template<is_iterable T>
	requires (!pod<typename T::value_type> || !std::ranges::contiguous_range<T>)
	binary_stream_writer& operator<<(T& data) {
		for(auto& element : data) {
			*this << element;
		}

		return *this;
	}

	template<is_iterable T>
	requires (!std::is_same_v<std::decay_t<T>, std::string> && !std::is_same_v<std::decay_t<T>, std::string_view>)
	binary_stream_writer& operator<<(prefixed<T> adaptor) {
		const auto count = endian::native_to_little(static_cast<std::uint32_t>(adaptor->size()));
		write(&count, sizeof(count));
		write_container(adaptor.str);
		return *this;
	}

	template<is_iterable T>
	requires (!std::is_same_v<std::decay_t<T>, std::string> && !std::is_same_v<std::decay_t<T>, std::string_view>)
	binary_stream_writer& operator<<(prefixed_varint<T> adaptor) {
		varint_encode(*this, adaptor->size());
		write(adaptor->data(), adaptor->size());
		write_container(adaptor.str);
		return *this;
	}

	/**
	 * @brief Writes a contiguous range to the stream.
	 * 
	 * @param data The contiguous range to be written to the stream.
	 */
	template<std::ranges::contiguous_range range>
	void put(range& data) {
		const auto write_size = data.size() * sizeof(typename range::value_type);
		write(data.data(), write_size);
	}

	/**
	 * @brief Writes a the provided value to the stream.
	 * 
	 * @param data The value to be written to the stream.
	 */
	void put(const arithmetic auto& data) {
		write(&data, sizeof(data));
	}

	/**
	 * @brief Writes data to the stream.
	 * 
	 * @param data The element to be written to the stream.
	 */
	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	void put(const endian_func& adaptor) {
		const auto swapped = adaptor.to();
		write(&swapped, sizeof(swapped));
	}

	/**
	 * @brief Writes count elements from the provided buffer to the stream.
	 * 
	 * @param data Pointer to the buffer from which data will be copied to the stream.
	 * @param count The number of elements to write.
	 */
	template<pod T>
	void put(const T* data, std::size_t count) {
		assert(data);
		const auto write_size = count * sizeof(T);
		write(data, write_size);
	}

	/**
	 * @brief Writes the data from the iterator range to the stream.
	 * 
	 * @param begin Iterator to the beginning of the data.
	 * @param end Iterator to the end of the data.
	 */
	template<typename It>
	void put(It begin, const It end) {
		for(auto it = begin; it != end; ++it) {
			*this << *it;
		}
	}
	/**
	 * @brief Allows for writing a provided byte value a specified number of times to
	 * the stream.
	 * 
	 * @param The byte value that will fill the specified number of bytes.
	 */
	template<std::size_t size>
	void fill(const std::uint8_t value) {
		const auto filled = generate_filled<size>(value);
		write(filled.data(), filled.size());
	}

	/**  Misc functions **/ 

	/**
	 * @brief Determines whether this container can write seek.
	 * 
	 * @return Whether this container is capable of write seeking.
	 */
	bool can_write_seek() const {
		return buffer_.can_write_seek();
	}

	/**
	 * @brief Performs write seeking within the container.
	 * 
	 * @param direction Specify whether to seek in a given direction or to absolute seek.
	 * @param offset The offset relative to the seek direction or the absolute value
	 * when using absolute seeking.
	 */
	void write_seek(const stream_seek direction, const std::size_t offset) {
		if(direction == stream_seek::sk_stream_absolute) {
			if(offset >= total_write_) {
				buffer_.write_seek(buffer_seek::sk_forward, offset - total_write_);
			} else {
				buffer_.write_seek(buffer_seek::sk_backward, total_write_ - offset);
			}

			total_write_ = offset;
		} else {
			buffer_.write_seek(static_cast<buffer_seek>(direction), offset);
		}
	}

	/**
	 * @brief Returns the size of the container.
	 * 
	 * @return The number of bytes of data available to read within the stream.
	 */
	std::size_t size() const {
		return buffer_.size();
	}

	/**
	 * @brief Whether the container is empty.
	 * 
	 * @return Returns true if the container is empty (has no data to be read).
	 */
	[[nodiscard]]
	bool empty() const {
		return buffer_.empty();
	}

	/**
	 * @return The total number of bytes written to the stream.
	 */
	std::size_t total_write() const {
		return total_write_;
	}

	/** 
	 * @brief Get a pointer to the buffer.
	 *
	 * @return Pointer to the underlying buffer. 
	 */
	buffer_write* buffer() {
		return &buffer_;
	}

	/** 
	 * @brief Get a pointer to the buffer.
	 *
	 * @return Pointer to the underlying buffer. 
	 */
	const buffer_write* buffer() const {
		return &buffer_;
	}
};

} // pmc, hexi

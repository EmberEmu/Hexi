//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/pmc/stream_base.h>
#include <hexi/pmc/buffer_read.h>
#include <hexi/concepts.h>
#include <hexi/endian.h>
#include <hexi/exception.h>
#include <hexi/shared.h>
#include <hexi/stream_adaptors.h>
#include <ranges>
#include <string>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace hexi::pmc {

using namespace detail;

class binary_stream_reader : virtual public stream_base {
	buffer_read& buffer_;
	std::size_t total_read_;
	const std::size_t read_limit_;

	void enforce_read_bounds(std::size_t read_size) {
		if(read_size > buffer_.size()) [[unlikely]] {
			set_state(stream_state::buff_limit_err);
			HEXI_THROW(buffer_underrun(read_size, total_read_, buffer_.size()));
		}

		if(read_limit_) {
			const auto max_read_remaining = read_limit_ - total_read_;

			if(read_size > max_read_remaining) [[unlikely]] {
				set_state(stream_state::read_limit_err);
				HEXI_THROW(stream_read_limit(read_size, total_read_, read_limit_));
			}
		}

		total_read_ += read_size;
	}

	inline void read(void* dest, const std::size_t size) {
		if(state() == stream_state::ok) [[likely]] {
			enforce_read_bounds(size);
			buffer_.read(dest, size);
		}
	}

public:
	explicit binary_stream_reader(buffer_read& source, std::size_t read_limit = 0)
		: stream_base(source),
		  buffer_(source),
		  total_read_(0),
		  read_limit_(read_limit) {}

	binary_stream_reader(binary_stream_reader&& rhs) noexcept
		: stream_base(rhs),
		  buffer_(rhs.buffer_), 
		  total_read_(rhs.total_read_),
		  read_limit_(rhs.read_limit_) {
		rhs.total_read_ = static_cast<std::size_t>(-1);
		rhs.set_state(stream_state::invalid_stream);
	}

	binary_stream_reader& operator=(binary_stream_reader&&) = delete;
	binary_stream_reader& operator=(const binary_stream_reader&) = delete;
	binary_stream_reader(const binary_stream_reader&) = delete;

	void deserialise(auto& object) {
		stream_read_adaptor adaptor(*this);
		object.serialise(adaptor);
	}

	binary_stream_reader& operator>>(prefixed<std::string> adaptor) {
		std::uint32_t size = 0;
		*this >> endian::le(size);

		if(state() != stream_state::ok) {
			return *this;
		}

		enforce_read_bounds(size);

		adaptor->resize_and_overwrite(size, [&](char* strbuf, std::size_t size) {
			buffer_.read(strbuf, size);
			return size;
		});

		return *this;
	}
	
	binary_stream_reader& operator>>(prefixed_varint<std::string> adaptor) {
		const auto size = varint_decode<std::size_t>(*this);

		// if an error was triggered during decode, we shouldn't reach here
		if(state() != stream_state::ok) {
			std::unreachable();
		}

		enforce_read_bounds(size);

		adaptor->resize_and_overwrite(size, [&](char* strbuf, std::size_t size) {
			buffer_.read(strbuf, size);
			return size;
		});

		return *this;
	}

	binary_stream_reader& operator>>(null_terminated<std::string> adaptor) {
		auto pos = buffer_.find_first_of(std::byte{0});

		if(pos == buffer_.npos) {
			adaptor->clear();
			return *this;
		}

		enforce_read_bounds(pos + 1); // include null terminator

		adaptor->resize_and_overwrite(pos, [&](char* strbuf, std::size_t size) {
			buffer_.read(strbuf, pos);
			return size;
		});

		buffer_.skip(1); // skip null terminator
		return *this;
	}

	binary_stream_reader& operator >>(std::string& data) {
		return (*this >> prefixed(data));
	}

	binary_stream_reader& operator>>(has_shr_override<binary_stream_reader> auto&& data) {
		return data.operator>>(*this);
	}

	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	binary_stream_reader& operator>>(endian_func adaptor) {
		read(&adaptor.value, sizeof(adaptor.value));
		adaptor.value = adaptor.from();
		return *this;
	}

	template<pod T>
	requires (!has_shr_override<T, binary_stream_reader>)
	binary_stream_reader& operator>>(T& data) {
		read(&data, sizeof(data));
		return *this;
	}

	binary_stream_reader& operator>>(pod auto& data) {
		read(&data, sizeof(data));
		return *this;
	}

	/**
	 * @brief Reads a string from the stream.
	 * 
	 * @param dest The destination string.
	 */
	void get(std::string& dest) {
		*this >> dest;
	}

	/**
	 * @brief Reads a fixed-length string from the stream.
	 * 
	 * @param dest The destination string.
	 * @param count The number of bytes to be read.
	 */
	void get(std::string& dest, std::size_t size) {
		enforce_read_bounds(size);

		dest.resize_and_overwrite(size, [&](char* strbuf, std::size_t len) {
			buffer_.read(strbuf, len);
			return len;
		});
	}

	/**
	 * @brief Read data from the stream into the provided destination argument.
	 * 
	 * @param dest The destination buffer.
	 * @param count The number of bytes to be read into the destination.
	 */
	template<typename T>
	void get(T* dest, std::size_t count) {
		assert(dest);
		const auto read_size = count * sizeof(T);
		read(dest, read_size);
	}

	/**
	 * @brief Read data from the stream to the destination represented by the iterators.
	 * 
	 * @param begin The beginning iterator.
	 * @param end The end iterator.
	 */
	template<typename It>
	void get(It begin, const It end) {
		for(; begin != end; ++begin) {
			*this >> *begin;
		}
	}

	/**
	 * @brief Read data from the stream into the provided destination argument.
	 * 
	 * @param dest A contiguous range into which the data should be read.
	 */
	template<std::ranges::contiguous_range range>
	void get(range& dest) {
		const auto read_size = dest.size() * sizeof(typename range::value_type);
		read(dest.data(), read_size);
	}

	/**
	 * @brief Read an arithmetic type from the stream.
	 * 
	 * @return The arithmetic value.
	 */
	template<arithmetic T>
	T get() {
		T t{};
		read(&t, sizeof(T));
		return t;
	}

	/**
	 * @brief Read an arithmetic type from the stream.
	 * 
	 * @return The arithmetic value.
	 */
	void get(arithmetic auto& dest) {
		read(&dest, sizeof(dest));
	}

	/**
	 * @brief Read an arithmetic type from the stream, allowing for endian
	 * conversion.
	 * 
	 * @param The destination for the read value.
	 */
	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	void get(endian_func& adaptor) {
		read(&adaptor.value, sizeof(adaptor.value));
		adaptor.value = adaptor.from();
	}

	/**
	 * @brief Read an arithmetic type from the stream, allowing for endian
	 * conversion.
	 * 
	 * @return The arithmetic value.
	 */
	template<arithmetic T, endian::conversion conversion>
	T get() {
		T t{};
		read(&t, sizeof(T));
		return endian::convert<conversion>(t);
	}

	/**  Misc functions **/ 

	/**
	 * @brief Skip over count bytes
	 *
	 * Skips over a number of bytes from the container. This should be used
	 * if the container holds data that you don't care about but don't want
	 * to have to read it to another buffer to move beyond it.
	 * 
	 * @param length The number of bytes to skip.
	 */
	void skip(std::size_t count) {
		enforce_read_bounds(count);
		buffer_.skip(count);
	}

	/**
	 * @return The total number of bytes read from the stream.
	 */
	std::size_t total_read() const {
		return total_read_;
	}

	/**
	 * @return If provided to the constructor, the upper limit on how much data
	 * can be read from this stream before an error is triggers.
	 */
	std::size_t read_limit() const {
		return read_limit_;
	}

	/**
	 * @brief Determine the maximum number of bytes that can be
	 * safely read from this stream.
	 * 
	 * The value returned may be lower than the amount of data
	 * available in the buffer if a read limit was set during
	 * the stream's construction.
	 * 
	 * @return The number of bytes available for reading.
	 */
	std::size_t read_max() const {
		if(read_limit_) {
			return read_limit_ - total_read_;
		} else {
			return buffer_.size();
		}
	}

	/**
	 * @return Pointer to stream's underlying buffer.
	 */
	buffer_read* buffer() const {
		return &buffer_;
	}
};

} // pmc, hexi

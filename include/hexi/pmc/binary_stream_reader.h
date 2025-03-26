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
#include <algorithm>
#include <ranges>
#include <string>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace hexi::pmc {

using namespace detail;

class binary_stream_reader : virtual public stream_base {
	buffer_read& buffer_;
	std::size_t total_read_;
	const std::size_t read_limit_;

	void check_read_bounds(std::size_t read_size) {
		if(read_size > buffer_.size()) [[unlikely]] {
			set_state(stream_state::buff_limit_err);
			throw buffer_underrun(read_size, total_read_, buffer_.size());
		}

		const auto req_total_read = total_read_ + read_size;

		if(read_limit_ && req_total_read > read_limit_) [[unlikely]] {
			set_state(stream_state::read_limit_err);
			throw stream_read_limit(read_size, total_read_, read_limit_);
		}

		total_read_ = req_total_read;
	}

public:
	explicit binary_stream_reader(buffer_read& source, std::size_t read_limit = 0)
		: stream_base(source),
		  buffer_(source),
		  total_read_(0),
		  read_limit_(read_limit) {}

	binary_stream_reader(binary_stream_reader&& rhs) = delete;
	binary_stream_reader& operator=(binary_stream_reader&&) = delete;
	binary_stream_reader& operator=(const binary_stream_reader&) = delete;
	binary_stream_reader(const binary_stream_reader&) = delete;

	// terminates when it hits a null byte, empty string if none found
	binary_stream_reader& operator>>(std::string& dest) {
		check_read_bounds(1); // just to prevent trying to read from an empty buffer
		auto pos = buffer_.find_first_of(std::byte(0));

		if(pos == buffer_read::npos) {
			dest.clear();
			return *this;
		}

		dest.resize_and_overwrite(pos, [&](char* strbuf, std::size_t size) {
			total_read_ += size;
			buffer_.read(strbuf, size);
			return size;
		});

		buffer_.skip(1); // skip null term
		return *this;
	}

	binary_stream_reader& operator>>(has_shr_override<binary_stream_reader> auto&& data) {
		return data.operator>>(*this);
	}

	template<pod T>
	requires (!has_shr_override<T, binary_stream_reader>)
	binary_stream_reader& operator>>(T& data) {
		check_read_bounds(sizeof(data));
		buffer_.read(&data, sizeof(data));
		return *this;
	}

	binary_stream_reader& operator>>(pod auto& data) {
		check_read_bounds(sizeof(data));
		buffer_.read(&data, sizeof(data));
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
	 * @param The number of bytes to be read.
	 */
	void get(std::string& dest, std::size_t size) {
		check_read_bounds(size);
		dest.resize_and_overwrite(size, [&](char* strbuf, std::size_t len) {
			buffer_.read(strbuf, len);
			return len;
		});
	}

	/**
	 * @brief Read data from the stream into the provided destination argument.
	 * 
	 * @param dest The destination buffer.
	 * @param The number of bytes to be read into the destination.
	 */
	template<typename T>
	void get(T* dest, std::size_t count) {
		assert(dest);
		const auto read_size = count * sizeof(T);
		check_read_bounds(read_size);
		buffer_.read(dest, read_size);
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
		const auto read_size = dest.size() * sizeof(range::value_type);
		check_read_bounds(read_size);
		buffer_.read(dest.data(), read_size);
	}

	/**
	 * @brief Read an arithmetic type from the stream.
	 * 
	 * @return The arithmetic value.
	 */
	template<arithmetic T>
	T get() {
		check_read_bounds(sizeof(T));
		T t{};
		buffer_.read(&t, sizeof(T));
		return t;
	}

	/**
	 * @brief Read an arithmetic type from the stream.
	 * 
	 * @return The arithmetic value.
	 */
	void get(arithmetic auto& dest) {
		check_read_bounds(sizeof(dest));
		buffer_.read(&dest, sizeof(dest));
	}

	/**
	 * @brief Read an arithmetic type from the stream, allowing for endian
	 * conversion.
	 * 
	 * @param The destination for the read value.
	 */
	template<endian::conversion conversion>
	void get(arithmetic auto& dest) {
		check_read_bounds(sizeof(dest));
		buffer_.read(&dest, sizeof(dest));
		dest = endian::convert<conversion>(dest);
	}

	/**
	 * @brief Read an arithmetic type from the stream, allowing for endian
	 * conversion.
	 * 
	 * @return The arithmetic value.
	 */
	template<arithmetic T, endian::conversion conversion>
	T get() {
		check_read_bounds(sizeof(T));
		T t{};
		buffer_.read(&t, sizeof(T));
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
		check_read_bounds(count);
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
	 * @return Pointer to stream's underlying buffer.
	 */
	buffer_read* buffer() const {
		return &buffer_;
	}
};

} // pmc, hexi

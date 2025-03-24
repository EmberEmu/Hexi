//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/pmc/stream_base.h>
#include <hexi/pmc/buffer_read.h>
#include <hexi/exception.h>
#include <hexi/shared.h>
#include <hexi/concepts.h>
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

	void get(std::string& dest) {
		*this >> dest;
	}

	void get(std::string& dest, std::size_t size) {
		check_read_bounds(size);
		dest.resize_and_overwrite(size, [&](char* strbuf, std::size_t len) {
			buffer_.read(strbuf, len);
			return len;
		});
	}

	template<typename T>
	void get(T* dest, std::size_t count) {
		assert(dest);
		const auto read_size = count * sizeof(T);
		check_read_bounds(read_size);
		buffer_.read(dest, read_size);
	}

	template<typename It>
	void get(It begin, const It end) {
		for(; begin != end; ++begin) {
			*this >> *begin;
		}
	}

	template<std::ranges::contiguous_range range>
	void get(range& dest) {
		const auto read_size = dest.size() * sizeof(range::value_type);
		check_read_bounds(read_size);
		buffer_.read(dest.data(), read_size);
	}

	/**  Misc functions **/ 

	void skip(std::size_t count) {
		check_read_bounds(count);
		buffer_.skip(count);
	}

	std::size_t total_read() const {
		return total_read_;
	}

	std::size_t read_limit() const {
		return read_limit_;
	}

	buffer_read* buffer() const {
		return &buffer_;
	}
};

} // pmc, hexi

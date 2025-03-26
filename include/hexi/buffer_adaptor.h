//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/shared.h>
#include <hexi/concepts.h>
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

	buffer_adaptor(buffer_adaptor&& rhs) = delete;
	buffer_adaptor& operator=(buffer_adaptor&&) = delete;
	buffer_adaptor& operator=(const buffer_adaptor&) = delete;
	buffer_adaptor(const buffer_adaptor&) = delete;

	template<typename T>
	void read(T* destination) {
		read(destination, sizeof(T));
	}

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

	template<typename T>
	void copy(T* destination) const {
		copy(destination, sizeof(T));
	}

	void copy(void* destination, size_type length) const {
		assert(destination);
		assert(!region_overlap(buffer_.data(), buffer_.size(), destination, length));
		std::memcpy(destination, read_ptr(), length);
	}

	void skip(size_type length) {
		read_ += length;

		if constexpr(space_optimise) {
			if(read_ == write_) {
				read_ = write_ = 0;
			}
		}
	}

	void write(const auto& source) requires(has_resize<buf_type>) {
		write(source, sizeof(source));
	}

	void write(const void* source, size_type length) requires(has_resize<buf_type>) {
		assert(source && !region_overlap(source, length, buffer_.data(), buffer_.size()));
		const auto min_req_size = write_ + length;

		// we don't use std::back_inserter so we can support seeks
		if(buffer_.size() < min_req_size) {
			if constexpr(has_resize_overwrite<buf_type>) {
				buffer_.resize_and_overwrite(min_req_size, [](char*, size_type size) {
					return size;
				});
			} else {
				buffer_.resize(min_req_size);
			}
		}

		std::memcpy(write_ptr(), source, length);
		write_ += length;
	}

	size_type find_first_of(value_type val) const {
		const auto data = read_ptr();

		for(size_type i = 0, j = size(); i < j; ++i) {
			if(data[i] == val) {
				return i;
			}
		}

		return npos;
	}
	
	size_type size() const {
		return buffer_.size() - read_;
	}

	[[nodiscard]]
	bool empty() const {
		return read_ == write_;
	}

	value_type& operator[](const size_type index) {
		return read_ptr()[index];
	}

	const value_type& operator[](const size_type index) const {
		return read_ptr()[index];
	}

	constexpr static bool can_write_seek() requires(has_resize<buf_type>) {
		return std::is_same_v<seeking, supported>;
	}

	void write_seek(const buffer_seek direction, const offset_type offset) requires(has_resize<buf_type>) {
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

	const auto read_ptr() const {
		return buffer_.data() + read_;
	}

	auto read_ptr() {
		return buffer_.data() + read_;
	}

	const auto write_ptr() const requires(has_resize<buf_type>) {
		return buffer_.data() + write_;
	}

	auto write_ptr() requires(has_resize<buf_type>) {
		return buffer_.data() + write_;
	}

	const auto data() const {
		return buffer_.data() + read_;
	}

	auto data() {
		return buffer_.data() + read_;
	}

	const auto storage() const {
		return buffer_.data();
	}

	auto storage() {
		return buffer_.data();
	}

	void advance_write(size_type bytes) {
		assert(buffer_.size() >= (write_ + bytes));
		write_ += bytes;
	}
};

} // hexi

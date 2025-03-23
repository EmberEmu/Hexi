//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/pmr/buffer_write.h>
#include <hexi/shared.h>
#include <hexi/concepts.h>
#include <ranges>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace hexi::pmr {

using namespace detail;

template<byte_oriented buf_type>
requires std::ranges::contiguous_range<buf_type>
class buffer_write_adaptor : public buffer_write {
	buf_type& buffer_;
	std::size_t write_;

public:
	buffer_write_adaptor(buf_type& buffer)
		: buffer_(buffer),
		  write_(buffer.size()) {}

	void write(auto& source) {
		write(&source, sizeof(source));
	}

	void write(const void* source, std::size_t length) override {
		assert(source && !region_overlap(source, length, buffer_.data(), buffer_.size()));
		const auto min_req_size = write_ + length;

		// we don't use std::back_inserter so we can support seeks
		if(buffer_.size() < min_req_size) {
			if constexpr(has_resize_overwrite<buf_type>) {
				buffer_.resize_and_overwrite(min_req_size, [](char*, std::size_t size) {
					return size;
				});
			} else {
				buffer_.resize(min_req_size);
			}
		}

		std::memcpy(buffer_.data() + write_, source, length);
		write_ += length;
	}

	void reserve(const std::size_t length) override {
		buffer_.reserve(length);
	}

	bool can_write_seek() const override {
		return true;
	}

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

	const auto storage() const {
		return buffer_.data();
	}

	auto storage() {
		return buffer_.data();
	}

	auto write_ptr() {
		return buffer_.data() + write_;
	}

	const auto write_ptr() const {
		return buffer_.data() + write_;
	}
	
	void reset() {
		write_ = 0;
		buffer_.clear();
	}
};

} // pmr, hexi

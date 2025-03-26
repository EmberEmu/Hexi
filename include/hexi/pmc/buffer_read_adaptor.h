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
	std::size_t read_;

public:
	buffer_read_adaptor(buf_type& buffer)
		: buffer_(buffer),
		  read_(0) {}

	template<typename T>
	void read(T* destination) {
		read(destination, sizeof(T));
	}

	void read(void* destination, std::size_t length) override {
		assert(destination && !region_overlap(buffer_.data(), buffer_.size(), destination, length));
		std::memcpy(destination, buffer_.data() + read_, length);
		read_ += length;
	}

	template<typename T>
	void copy(T* destination) const {
		copy(destination, sizeof(T));
	}

	void copy(void* destination, std::size_t length) const override {
		assert(destination && !region_overlap(buffer_.data(), buffer_.size(), destination, length));
		std::memcpy(destination, buffer_.data() + read_, length);
	}

	void skip(std::size_t length) override {
		read_ += length;
	}

	std::size_t size() const override {
		return buffer_.size() - read_;
	}

	[[nodiscard]]
	bool empty() const override {
		return !(buffer_.size() - read_);
	}

	const std::byte& operator[](const std::size_t index) const override {
		return reinterpret_cast<const std::byte*>(buffer_.data() + read_)[index];
	}

	const auto read_ptr() const {
		return buffer_.data() + read_;
	}

	const auto read_offset() const {
		return read_;
	}

	std::size_t find_first_of(std::byte val) const override {
		for(auto i = read_; i < size(); ++i) {
			if(static_cast<std::byte>(buffer_[i]) == val) {
				return i - read_;
			}
		}

		return npos;
	}

	void clear() {
		read_ = 0;
		buffer_.clear();
	}
};

} // pmc, hexi

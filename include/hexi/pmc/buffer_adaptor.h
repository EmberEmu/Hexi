//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/pmc/buffer.h>
#include <hexi/pmc/buffer_read_adaptor.h>
#include <hexi/pmc/buffer_write_adaptor.h>
#include <hexi/concepts.h>
#include <ranges>

namespace hexi::pmc {

template<byte_oriented buf_type, bool allow_optimise  = true>
requires std::ranges::contiguous_range<buf_type>
class buffer_adaptor final : public buffer_read_adaptor<buf_type>,
                             public buffer_write_adaptor<buf_type>,
                             public buffer {
	void reset() {
		if(buffer_read_adaptor<buf_type>::read_ptr() == buffer_write_adaptor<buf_type>::write_ptr()) {
			buffer_read_adaptor<buf_type>::reset();
			buffer_write_adaptor<buf_type>::reset();
		}
	}
public:
	explicit buffer_adaptor(buf_type& buffer)
		: buffer_read_adaptor<buf_type>(buffer),
		  buffer_write_adaptor<buf_type>(buffer) {}

	template<typename T>
	void read(T* destination) {
		buffer_read_adaptor<buf_type>::read(destination);

		if constexpr(allow_optimise) {
			reset();
		}
	}

	void read(void* destination, std::size_t length) override {
		buffer_read_adaptor<buf_type>::read(destination, length);

		if constexpr(allow_optimise) {
			reset();
		}
	};

	void write(const auto& source) {
		buffer_write_adaptor<buf_type>::write(source);
	};

	void write(const void* source, std::size_t length) override {
		buffer_write_adaptor<buf_type>::write(source, length);
	};

	void copy(auto* destination) const {
		buffer_read_adaptor<buf_type>::copy(destination);
	};

	void copy(void* destination, std::size_t length) const override {
		buffer_read_adaptor<buf_type>::copy(destination, length);
	};

	void skip(std::size_t length) override {
		buffer_read_adaptor<buf_type>::skip(length);

		if constexpr(allow_optimise) {
			reset();
		}
	};

	const std::byte& operator[](const std::size_t index) const override { 
		return buffer_read_adaptor<buf_type>::operator[](index); 
	};

	std::byte& operator[](const std::size_t index) override {
		const auto offset = buffer_read_adaptor<buf_type>::read_offset();
		auto buffer = buffer_write_adaptor<buf_type>::storage();
		return reinterpret_cast<std::byte*>(buffer + offset)[index];
	}

	void reserve(std::size_t length) override {
		buffer_write_adaptor<buf_type>::reserve(length);
	};

	bool can_write_seek() const override { 
		return buffer_write_adaptor<buf_type>::can_write_seek();
	};

	void write_seek(buffer_seek direction, std::size_t offset) override {
		buffer_write_adaptor<buf_type>::write_seek(direction, offset);
	};

	std::size_t size() const override { 
		return buffer_read_adaptor<buf_type>::size(); 
	};

	[[nodiscard]]
	bool empty() const override { 
		return buffer_read_adaptor<buf_type>::empty();
	}

	std::size_t find_first_of(std::byte val) const override { 
		return buffer_read_adaptor<buf_type>::find_first_of(val);
	}
};

} // pmc, hexi

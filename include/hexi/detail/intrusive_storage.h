//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#if _MSC_VER
#pragma warning(disable : 4996)
#endif

#include <hexi/shared.h>
#include <hexi/concepts.h>
#include <array>
#include <concepts>
#include <type_traits>
#include <cassert>
#include <cstring>
#include <cstddef>

namespace hexi::detail {

struct intrusive_node {
	intrusive_node* next;
	intrusive_node* prev;
};

template<std::size_t block_size, byte_type storage_type = std::byte>
struct intrusive_storage final {
	using value_type = storage_type;
	using OffsetType = std::remove_const_t<decltype(block_size)>;

	OffsetType read_offset = 0;
	OffsetType write_offset = 0;
	intrusive_node node {};
	std::array<value_type, block_size> storage;

	void reset() {
		read_offset = 0;
		write_offset = 0;
	}

	std::size_t write(const auto source, std::size_t length) {
		assert(!region_overlap(source, length, storage.data(), storage.size()));
		std::size_t write_len = block_size - write_offset;

		if(write_len > length) {
			write_len = length;
		}

		std::memcpy(storage.data() + write_offset, source, write_len);
		write_offset += static_cast<OffsetType>(write_len);
		return write_len;
	}

	std::size_t copy(auto destination, const std::size_t length) const {
		assert(!region_overlap(storage.data(), storage.size(), destination, length));
		std::size_t read_len = block_size - read_offset;

		if(read_len > length) {
			read_len = length;
		}

		std::memcpy(destination, storage.data() + read_offset, read_len);
		return read_len;
	}

	std::size_t read(auto destination, const std::size_t length, const bool allow_optimise = false) {
		std::size_t read_len = copy(destination, length);
		read_offset += static_cast<OffsetType>(read_len);

		if(read_offset == write_offset && allow_optimise) {
			reset();
		}

		return read_len;
	}

	std::size_t skip(const std::size_t length, const bool allow_optimise = false) {
		std::size_t skip_len = block_size - read_offset;

		if(skip_len > length) {
			skip_len = length;
		}

		read_offset += static_cast<OffsetType>(skip_len);

		if(read_offset == write_offset && allow_optimise) {
			reset();
		}

		return skip_len;
	}

	std::size_t size() const {
		return write_offset - read_offset;
	}

	std::size_t free() const {
		return block_size - write_offset;
	}

	void write_seek(const buffer_seek direction, const std::size_t offset) {
		switch(direction) {
			case buffer_seek::sk_absolute:
				write_offset = offset;
				break;
			case buffer_seek::sk_backward:
				write_offset -= static_cast<OffsetType>(offset);
				break;
			case buffer_seek::sk_forward:
				write_offset += static_cast<OffsetType>(offset);
				break;
		}
	}

	std::size_t advance_write(std::size_t size) {
		const auto remaining = free();

		if(remaining < size) {
			size = remaining;
		}

		write_offset += static_cast<OffsetType>(size);
		return size;
	}

	const value_type* read_data() const {
		return storage.data() + read_offset;
	}

	value_type* write_data() {
		return storage.data() + write_offset;
	}

	value_type& operator[](const std::size_t index) {
		return *(storage.data() + index);
	}

	value_type& operator[](const std::size_t index) const {
		return *(storage.data() + index);
	}
};

} // detail, hexi

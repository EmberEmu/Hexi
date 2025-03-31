//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <type_traits>
#include <cstddef>
#include <cstdint>

namespace hexi {

struct is_contiguous {};
struct is_non_contiguous {};
struct supported {};
struct unsupported {};
struct except_tag {};
struct allow_throw_t : except_tag {};
struct no_throw_t : except_tag {};

constexpr static no_throw_t no_throw {};
constexpr static allow_throw_t allow_throw {};

struct init_empty_t {};
constexpr static init_empty_t init_empty {};

#define STRING_ADAPTOR(adaptor_name)                      \
template<typename string_type>                            \
struct adaptor_name {                                     \
    string_type& str;                                     \
    string_type* operator->() { return &str; }            \
};                                                        \
/* deduction guide required for clang 17 support */       \
template<typename string_type>                            \
adaptor_name(string_type&) -> adaptor_name<string_type>;  \

STRING_ADAPTOR(raw)
STRING_ADAPTOR(prefixed)
STRING_ADAPTOR(prefixed_varint)
STRING_ADAPTOR(null_terminated)

enum class buffer_seek {
	sk_absolute, sk_backward, sk_forward
};

enum class stream_seek {
	// Seeks within the entire underlying buffer
	sk_buffer_absolute,
	sk_backward,
	sk_forward,
	// Seeks only within the range written by the current stream
	sk_stream_absolute
};

enum class stream_state {
	ok,
	read_limit_err,
	buff_limit_err,
	buff_write_err,
	invalid_stream,
	user_defined_err
};

namespace detail {

template<typename size_type, typename stream_type>
constexpr auto varint_decode(stream_type& stream) -> std::pair<bool, size_type> {
	int shift { 0 };
	size_type value { 0 };
	std::uint8_t byte { 0 };

	do {
		// if reading another byte would violate the read limit
		if(stream.read_max() == 0) {
			return { false, 0 };
		}

		stream.get(&byte, 1);
		value |= (static_cast<size_type>(byte & 0x7f) << shift);
		shift += 7;
	} while(byte & 0x80);

	return { true, value };
}

template<typename size_type, typename stream_type>
constexpr auto varint_encode(stream_type& stream, size_type value) -> size_type {
	size_type written = 0;

	while(value > 0x7f) {
		const std::uint8_t byte = (value & 0x7f) | 0x80;
		stream.put(&byte, 1);
		value >>= 7;
		++written;
	}

	const std::uint8_t byte = value & 0x7f;
	stream.put(&byte, 1);
	return ++written;
}

template<decltype(auto) size>
static constexpr auto generate_filled(const std::uint8_t value) {
	std::array<std::uint8_t, size> target{};
	std::ranges::fill(target, value);
	return target;
}

// Returns true if there's any overlap between source and destination ranges
static inline bool region_overlap(const void* src, std::size_t src_len, const void* dst, std::size_t dst_len) {
	const auto src_beg = std::bit_cast<std::uintptr_t>(src);
	const auto src_end = src_beg + src_len;
	const auto dst_beg = std::bit_cast<std::uintptr_t>(dst);
	const auto dst_end = dst_beg + dst_len;

	// cannot assume src is before dst or vice versa
	return (src_beg >= dst_beg && src_beg < dst_end)
		|| (src_end > dst_beg && src_end <= dst_end)
		|| (dst_beg >= src_beg && dst_beg < src_end)
		|| (dst_end > src_beg && dst_end <= src_end);
}

} // detail

} // hexi

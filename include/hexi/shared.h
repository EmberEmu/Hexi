//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

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
struct except_tag{};
struct allow_throw : except_tag{};
struct no_throw : except_tag{};

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
	user_defined_err
};

namespace detail {

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

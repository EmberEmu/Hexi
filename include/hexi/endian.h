//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/concepts.h>
#include <bit>
#include <type_traits>
#include <utility>
#include <cstdint>

namespace hexi::endian {

enum class conversion {
	big_to_native,
	native_to_big,
	little_to_native,
	native_to_little
};

constexpr auto conditional_reverse(arithmetic auto value, std::endian from, std::endian to) {
	using type = decltype(value);

	if(from != to) {
		if constexpr(std::is_same_v<type, float>) {
			value = std::bit_cast<type>(std::byteswap(std::bit_cast<std::uint32_t>(value)));
		} else if constexpr(std::is_same_v<type, double>) {
			value = std::bit_cast<type>(std::byteswap(std::bit_cast<std::uint64_t>(value)));
		} else {
			value = std::byteswap(value);
		}
	}

	return value;
}

template<std::endian from, std::endian to>
constexpr auto conditional_reverse(arithmetic auto value) {
	using type = decltype(value);

	if constexpr(from != to) {
		if constexpr(std::is_same_v<type, float>) {
			value = std::bit_cast<type>(std::byteswap(std::bit_cast<std::uint32_t>(value)));
		} else if constexpr(std::is_same_v<type, double>) {
			value = std::bit_cast<type>(std::byteswap(std::bit_cast<std::uint64_t>(value)));
		} else {
			value = std::byteswap(value);
		}
	}

	return value;
}

constexpr auto little_to_native(arithmetic auto value) {
	return conditional_reverse<std::endian::little, std::endian::native>(value);
}

constexpr auto big_to_native(arithmetic auto value) {
	return conditional_reverse<std::endian::big, std::endian::native>(value);
}

constexpr auto native_to_little(arithmetic auto value) {
	return conditional_reverse<std::endian::native, std::endian::little>(value);
}

constexpr auto native_to_big(arithmetic auto value) {
	return conditional_reverse<std::endian::native, std::endian::big>(value);
}

template<std::endian from, std::endian to>
constexpr void conditional_reverse_inplace(arithmetic auto& value) {
	using type = std::remove_reference_t<decltype(value)>;

	if constexpr(from != to) {
		if constexpr(std::is_same_v<type, float>) {
			value = std::bit_cast<type>(std::byteswap(std::bit_cast<std::uint32_t>(value)));
		} else if constexpr(std::is_same_v<type, double>) {
			value = std::bit_cast<type>(std::byteswap(std::bit_cast<std::uint64_t>(value)));
		} else {
			value = std::byteswap(value);
		}
	}
}

constexpr void conditional_reverse_inplace(arithmetic auto& value, std::endian from, std::endian to) {
	using type = std::remove_reference_t<decltype(value)>;

	if(from != to) {
		if constexpr(std::is_same_v<type, float>) {
			value = std::bit_cast<type>(std::byteswap(std::bit_cast<std::uint32_t>(value)));
		} else if constexpr(std::is_same_v<type, double>) {
			value = std::bit_cast<type>(std::byteswap(std::bit_cast<std::uint64_t>(value)));
		} else {
			value = std::byteswap(value);
		}
	}
}

constexpr void little_to_native_inplace(arithmetic auto& value) {
	conditional_reverse_inplace<std::endian::little, std::endian::native>(value);
}

constexpr void big_to_native_inplace(arithmetic auto& value) {
	conditional_reverse_inplace<std::endian::big, std::endian::native>(value);
}

constexpr void native_to_little_inplace(arithmetic auto& value) {
	conditional_reverse_inplace<std::endian::native, std::endian::little>(value);
}

constexpr void native_to_big_inplace(arithmetic auto& value) {
	conditional_reverse_inplace<std::endian::native, std::endian::big>(value);
}

template<conversion _conversion>
constexpr auto convert(arithmetic auto value) -> decltype(value) {
	switch(_conversion) {
		case conversion::big_to_native:
			return big_to_native(value);
		case conversion::native_to_big:
			return native_to_big(value);
		case conversion::little_to_native:
			return little_to_native(value);
		case conversion::native_to_little:
			return native_to_little(value);
		default:
			std::unreachable();
	};
}

struct adaptor_in_tag_t {};
struct adaptor_out_tag_t {};

#define ENDIAN_ADAPTOR(name, func, tag, ref) \
template<arithmetic T>                       \
struct name final : tag {                    \
	T ref value;                             \
    name(T ref t) : value(t) {}              \
	auto convert() -> T {                    \
		return func(value);                  \
	}                                        \
};

#define ENDIAN_ADAPTOR_OUT(name, func) ENDIAN_ADAPTOR(name, func, adaptor_out_tag_t, &)
#define ENDIAN_ADAPTOR_IN(name, func)  ENDIAN_ADAPTOR(name, func, adaptor_in_tag_t,   )

ENDIAN_ADAPTOR_IN(to_big,       native_to_big)
ENDIAN_ADAPTOR_IN(to_little,    native_to_little)
ENDIAN_ADAPTOR_OUT(from_big,    big_to_native)
ENDIAN_ADAPTOR_OUT(from_little, little_to_native)

struct storage_tag {};
struct as_big_t final : storage_tag {};
struct as_little_t final : storage_tag {};
struct as_native_t final : storage_tag {};

constexpr static as_big_t big {};
constexpr static as_little_t little {};
constexpr static as_native_t native {};

inline auto storage_in(const arithmetic auto& value, as_native_t) {
	return value;
}

inline auto storage_in(const arithmetic auto& value, as_little_t) {
	return native_to_little(value);
}

inline auto storage_in(const arithmetic auto& value, as_big_t) {
	return native_to_big(value);
}

inline void storage_out(arithmetic auto& value, as_native_t) {}

inline void storage_out(arithmetic auto& value, as_little_t) {
	return little_to_native_inplace(value);
}

inline void storage_out(arithmetic auto& value, as_big_t) {
	return big_to_native_inplace(value);
}


} // endian, hexi
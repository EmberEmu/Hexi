//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/shared.h>
#include <bit>
#include <concepts>
#include <ranges>
#include <type_traits>

namespace hexi {

template<typename buf_type>
concept writeable =
	requires(buf_type t, void* v, typename buf_type::size_type s) {
		{ t.write(v, s) } -> std::same_as<void>;
};

template<typename buf_type>
concept seekable = requires(buf_type t) {
	std::is_same_v<typename buf_type::seeking, supported>;
};

template<typename buf_type>
concept contiguous = requires(buf_type t) {
	std::is_same_v<typename buf_type::contiguous, is_contiguous>;
};

template<typename T>
concept arithmetic = std::integral<T> || std::floating_point<T>;

template<typename T>
concept byte_type = sizeof(T) == 1;

template<typename T>
concept byte_oriented = byte_type<typename T::value_type>;

template<typename T>
concept pod = std::is_standard_layout_v<T> && std::is_trivial_v<T>;

template<typename T>
concept has_resize_overwrite =
	requires(T t) {
		{ t.resize_and_overwrite(typename T::size_type(), [](char*, T::size_type) {}) } -> std::same_as<void>;
};

template<typename T>
concept has_resize = 
	requires(T t) {
		{ t.resize(typename T::size_type() ) } -> std::same_as<void>;
};

template<typename T>
concept has_reserve = 
	requires(T t) {
		{ t.reserve(typename T::size_type() ) } -> std::same_as<void>;
};

template<typename T>
concept has_clear = 
	requires(T t) {
		{ t.clear() } -> std::same_as<void>;
};

template<typename T, typename U>
concept has_shl_override =
	requires(T t, U& u) {
		{ t.operator<<(u) } -> std::same_as<U&>;
};

template<typename T, typename U>
concept has_shr_override =
	requires(T t, U& u) {
		{ t.operator>>(u) } -> std::same_as<U&>;
};

template<typename T, typename U>
concept has_serialise =
	requires(T t, U& u) {
		{ t.serialise(u) } -> std::same_as<void>;
};

template<typename T>
concept is_iterable =
	requires(T t) {
		t.begin(); t.end();
};

} // hexi

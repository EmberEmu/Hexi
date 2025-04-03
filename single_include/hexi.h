//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

// #include <hexi/binary_stream.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/shared.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <type_traits>
#include <cstddef>
#include <cstdint>

namespace hexi {

#if defined(_EXCEPTIONS) || defined(__cpp_exceptions) || defined(_CPPUNWIND)
	#define HEXI_TRY try
	#define HEXI_CATCH(exception) catch(exception)
	#define HEXI_THROW(...) throw __VA_ARGS__
	#define HEXI_EXCEPTION_TAG allow_throw_t
#else
	#include <cstdlib> 
	#define HEXI_TRY if(true)  
	#define HEXI_CATCH(exception) if(false)
	#define HEXI_THROW(...) std::abort()
	#define HEXI_EXCEPTION_TAG no_throw_t
#endif

struct is_contiguous {};
struct is_non_contiguous {};
struct supported {};
struct unsupported {};
struct except_tag {};
struct allow_throw_t : except_tag {};
struct no_throw_t : except_tag {};

[[maybe_unused]] constexpr static no_throw_t no_throw {};
[[maybe_unused]] constexpr static allow_throw_t allow_throw {};

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
constexpr auto varint_decode(stream_type& stream) -> size_type {
	int shift { 0 };
	size_type value { 0 };
	std::uint8_t byte { 0 };

	do {
		byte = 0; // clear in case an error occurs
		stream.get(&byte, 1);
		value |= (static_cast<size_type>(byte & 0x7f) << shift);
		shift += 7;
	} while(byte & 0x80);

	return value;
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
[[maybe_unused]]
static inline bool region_overlap(const void* src, std::size_t src_len, const void* dst, std::size_t dst_len) {
	const auto src_beg = std::bit_cast<std::uintptr_t>(src);
	const auto src_end = src_beg + src_len;
	const auto dst_beg = std::bit_cast<std::uintptr_t>(dst);
	const auto dst_end = dst_beg + dst_len;
	return src_beg < dst_end && dst_beg < src_end;
}

} // detail

} // hexi

// #include <hexi/concepts.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/shared.h>

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

// #include <hexi/exception.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



#include <format>
#include <stdexcept>
#include <utility>
#include <cstddef>

namespace hexi {

class exception : public std::runtime_error {
public:
	exception(std::string msg)
		: std::runtime_error(std::move(msg)) {}
};

class buffer_underrun final : public exception {
public:
	const std::size_t buff_size, read_size, total_read;

	buffer_underrun(std::size_t read_size, std::size_t total_read, std::size_t buff_size)
		: exception(std::format(
			"Buffer underrun: {} byte read requested, buffer contains {} bytes and total bytes read was {}",
			read_size, buff_size, total_read)),
		buff_size(buff_size), read_size(read_size), total_read(total_read) {}
};

class buffer_overflow final : public exception {
public:
	const std::size_t free, write_size, total_write;

	buffer_overflow(std::size_t write_size, std::size_t total_write, std::size_t free)
		: exception(std::format(
			"Buffer overflow: {} byte write requested, free space is {} bytes and total bytes written was {}",
			write_size, free, total_write)),
		free(free), write_size(write_size), total_write(total_write) {}
};

class stream_read_limit final : public exception {
public:
	const std::size_t read_limit, read_size, total_read;

	stream_read_limit(std::size_t read_size, std::size_t total_read, std::size_t read_limit)
		: exception(std::format(
			"Read boundary exceeded: {} byte read requested, read limit was {} bytes and total bytes read was {}",
			read_size, read_limit, total_read)),
		read_limit(read_limit), read_size(read_size), total_read(total_read) {}
};

} // hexi

// #include <hexi/endian.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/concepts.h>

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

struct adaptor_tag_t {};

#define ENDIAN_ADAPTOR(name, func_to, func_from) \
template<arithmetic T>                           \
struct name final : adaptor_tag_t {              \
	T& value;                                    \
                                                 \
    name(T& t) : value(t) {}                     \
    name(T&& t) : value(t) {}                    \
                                                 \
	auto to() -> T {                             \
		return func_to(value);                   \
	}                                            \
	auto from() -> T {                           \
		return func_from(value);                 \
	}                                            \
};

ENDIAN_ADAPTOR(be, native_to_big,    big_to_native)
ENDIAN_ADAPTOR(le, native_to_little, little_to_native)

struct storage_tag {};
struct as_big_t final : storage_tag {};
struct as_little_t final : storage_tag {};
struct as_native_t final : storage_tag {};

[[maybe_unused]] constexpr static as_big_t big {};
[[maybe_unused]] constexpr static as_little_t little {};
[[maybe_unused]] constexpr static as_native_t native {};

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
// #include <hexi/stream_adaptors.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



#include <utility>

namespace hexi {

template<typename stream_type>
class stream_read_adaptor final {
	stream_type& _stream;

public:
	stream_read_adaptor(stream_type& stream) 
		: _stream(stream) {}

	void operator&(auto&& arg) {
		_stream >> arg;
	}

	template<typename ...Ts>
	void operator()(Ts&&... args) {
		(_stream >> ... >> args);
	}

	template<typename ...Ts>
	void forward(Ts&&... args) {
		_stream.get(std::forward<Ts>(args)...);
	}
};

template<typename stream_type>
class stream_write_adaptor final {
	stream_type& _stream;

public:
	stream_write_adaptor(stream_type& stream) 
		: _stream(stream) {}

	void operator&(auto&& arg) {
		_stream << arg;
	}

	template<typename ...Ts>
	void operator()(Ts&&... args) {
		(_stream << ... << args);
	}

	template<typename ...Ts>
	void forward(Ts&&... args) {
		_stream.put(std::forward<Ts>(args)...);
	}
};

} // hexi
#include <concepts>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace hexi {

using namespace detail;

#define STREAM_READ_BOUNDS_ENFORCE(read_size, ret_var)            \
	if(state_ != stream_state::ok) [[unlikely]] {                 \
		return ret_var;                                           \
	}                                                             \
                                                                  \
	enforce_read_bounds(read_size);                               \
	                                                              \
	if constexpr(std::is_same_v<exceptions, no_throw_t>) {        \
		if(state_ != stream_state::ok) [[unlikely]] {             \
			return ret_var;                                       \
		}                                                         \
	}

#define SAFE_READ(dest, read_size, ret_var)                       \
	STREAM_READ_BOUNDS_ENFORCE(read_size, ret_var)                \
	buffer_.read(dest, read_size);

template<
	byte_oriented buf_type,
	std::derived_from<except_tag> exceptions = HEXI_EXCEPTION_TAG,
	std::derived_from<endian::storage_tag> endianness = endian::as_native_t
>
class binary_stream final {
public:
	using size_type          = typename buf_type::size_type;
	using offset_type        = typename buf_type::offset_type;
	using seeking            = typename buf_type::seeking;
	using value_type         = typename buf_type::value_type;
	using contiguous_type    = typename buf_type::contiguous;
	
	static constexpr endianness byte_order{};

private:
	using cond_size_type = std::conditional_t<writeable<buf_type>, size_type, std::monostate>;

	buf_type& buffer_;
	[[no_unique_address]] cond_size_type total_write_{};
	size_type total_read_ = 0;
	stream_state state_ = stream_state::ok;
	const size_type read_limit_;

	inline void enforce_read_bounds(const size_type read_size) {
		if(read_size > buffer_.size()) [[unlikely]] {
			state_ = stream_state::buff_limit_err;

			if constexpr(std::is_same_v<exceptions, allow_throw_t>) {
				HEXI_THROW(buffer_underrun(read_size, total_read_, buffer_.size()));
			}

			return;
		}

		if(read_limit_) {
			const auto max_read_remaining = read_limit_ - total_read_;

			if(read_size > max_read_remaining) [[unlikely]] {
				state_ = stream_state::read_limit_err;

				if constexpr(std::is_same_v<exceptions, allow_throw_t>) {
					HEXI_THROW(stream_read_limit(read_size, total_read_, read_limit_));
				}

				return;
			}
		}

		total_read_ += read_size;
	}

	template<typename T>
	inline void advance_write(T&& arg) {
		total_write_ += sizeof(std::decay_t<T>);
	}

	template<typename T, typename U>
	inline void advance_write(T&&, U&& size) {
		total_write_ += size;
	}

	template<typename... Ts>
	inline void write(Ts&&... args) {
		HEXI_TRY {
			if(state_ == stream_state::ok) [[likely]] {
				buffer_.write(std::forward<Ts>(args)...);
				advance_write(std::forward<Ts>(args)...);                            
			}
		} HEXI_CATCH(...) {
			state_ = stream_state::buff_write_err;

			if constexpr(std::is_same_v<exceptions, allow_throw_t>) {
				HEXI_THROW();
			}
		}
	}

public:
	explicit binary_stream(buf_type& source, size_type read_limit = 0)
		: buffer_(source),
		  read_limit_(read_limit) {};

	explicit binary_stream(buf_type& source, exceptions)
		: binary_stream(source, 0) {}

	explicit binary_stream(buf_type& source, endianness)
		: binary_stream(source, 0) {}

	explicit binary_stream(buf_type& source, exceptions, endianness)
		: binary_stream(source, 0) {}

	explicit binary_stream(buf_type& source, size_type read_limit, exceptions)
		: binary_stream(source, read_limit) {}

	explicit binary_stream(buf_type& source, size_type read_limit, endianness)
		: binary_stream(source, read_limit) {}

	explicit binary_stream(buf_type& source, size_type read_limit, exceptions, endianness)
		: binary_stream(source, read_limit) {}

	binary_stream(binary_stream&& rhs) noexcept
		: buffer_(rhs.buffer_), 
		  total_write_(rhs.total_write_),
		  total_read_(rhs.total_read_),
		  state_(rhs.state_),
		  read_limit_(rhs.read_limit_) {
		rhs.total_read_ = static_cast<size_type>(-1);
		rhs.state_ = stream_state::invalid_stream;
	}

	binary_stream& operator=(binary_stream&&) = delete;
	binary_stream& operator=(binary_stream&) = delete;
	binary_stream(binary_stream&) = delete;

	/*** Write ***/

	void serialise(auto&& object) requires writeable<buf_type> {
		stream_write_adaptor adaptor(*this);
		object.serialise(adaptor);
	}

	template<typename T>
	requires has_serialise<T, stream_write_adaptor<binary_stream>>
	binary_stream& operator<<(T& data) requires writeable<buf_type> {
		serialise(data);
		return *this;
	}

	binary_stream& operator<<(const has_shl_override<binary_stream> auto& data)
	requires writeable<buf_type> {
		return data.operator<<(*this);
	}

	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	binary_stream& operator<<(endian_func adaptor) requires writeable<buf_type> {
		const auto converted = adaptor.to();
		write(&converted, sizeof(converted));
		return *this;
	}

	binary_stream& operator<<(const arithmetic auto& data) requires writeable<buf_type> {
		const auto converted = endian::storage_in(data, byte_order);
		write(&converted, sizeof(converted));
		return *this;
	}

	template<pod T>
	requires (!has_shl_override<T, binary_stream> && !arithmetic<T>)
	binary_stream& operator<<(const T& data) requires writeable<buf_type> {
		write(&data, sizeof(T));
		return *this;
	}

	template<typename T>
	binary_stream& operator<<(prefixed<T> adaptor) requires writeable<buf_type> {
		const auto size = static_cast<std::uint32_t>(adaptor->size());
		write(endian::native_to_little(size));
		write(adaptor->data(), static_cast<size_type>(size));
		return *this;
	}

	template<typename T>
	binary_stream& operator<<(prefixed_varint<T> adaptor) requires writeable<buf_type> {
		varint_encode(*this, adaptor->size());
		write(adaptor->data(), adaptor->size());
		return *this;
	}

	template<typename T>
	requires std::is_same_v<std::decay_t<T>, std::string_view>
	binary_stream& operator<<(null_terminated<T> adaptor) requires writeable<buf_type> {
		assert(adaptor->find_first_of('\0') == adaptor->npos);
		write(adaptor->data(), adaptor->size());
		write('\0');
		return *this;
	}

	template<typename T>
	requires std::is_same_v<std::decay_t<T>, std::string>
	binary_stream& operator<<(null_terminated<T> adaptor) requires writeable<buf_type> {
		assert(adaptor->find_first_of('\0') == adaptor->npos);
		write(adaptor->data(), adaptor->size() + 1); // yes, the standard allows this
		return *this;
	}

	template<typename T>
	binary_stream& operator<<(raw<T> adaptor) requires writeable<buf_type> {
		write(adaptor->data(), adaptor->size());
		return *this;
	}

	binary_stream& operator<<(std::string_view string) requires writeable<buf_type> {
		return (*this << prefixed(string));
	}

	binary_stream& operator<<(const std::string& string) requires writeable<buf_type> {
		return (*this << prefixed(string));
	}

	binary_stream& operator<<(const char* data) requires writeable<buf_type> {
		assert(data);
		const auto len = std::strlen(data);
		write(data, len + 1); // include terminator
		return *this;
	}

	template<std::ranges::contiguous_range range>
	requires pod<typename range::value_type>
	binary_stream& operator <<(const range& data) requires writeable<buf_type> {
		const auto write_size = data.size() * sizeof(typename range::value_type);
		write(data.data(), write_size);
		return *this;
	}

	template<is_iterable T>
	requires (!pod<typename T::value_type> || !std::ranges::contiguous_range<T>)
	binary_stream& operator<<(T& data) requires writeable<buf_type> {
		for(auto& element : data) {
           *this << element;
        }

		return *this;
	}

	/**
	 * @brief Writes a contiguous range to the stream.
	 * 
	 * @param data The contiguous range to be written to the stream.
	 */
	template<std::ranges::contiguous_range range>
	void put(const range& data) requires writeable<buf_type> {
		const auto write_size = data.size() * sizeof(typename range::value_type);
		write(data.data(), write_size);
	}

	/**
	 * @brief Writes a the provided value to the stream.
	 * 
	 * @param data The value to be written to the stream.
	 */
	void put(const arithmetic auto& data) requires writeable<buf_type> {
		write(&data, sizeof(data));
	}

	/**
	 * @brief Writes data to the stream.
	 * 
	 * @param data The element to be written to the stream.
	 */
	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	void put(const endian_func& adaptor) requires writeable<buf_type> {
		const auto swapped = adaptor.to();
		write(&swapped, sizeof(swapped));
	}

	/**
	 * @brief Writes count elements from the provided buffer to the stream.
	 * 
	 * @param data Pointer to the buffer from which data will be copied to the stream.
	 * @param count The number of elements to write.
	 */
	template<pod T>
	void put(const T* data, size_type count) requires writeable<buf_type> {
		const auto write_size = count * sizeof(T);
		write(data, write_size);
	}

	/**
	 * @brief Writes the data from the iterator range to the stream.
	 * 
	 * @param begin Iterator to the beginning of the data.
	 * @param end Iterator to the end of the data.
	 */
	template<typename It>
	void put(It begin, const It end) requires writeable<buf_type> {
		for(auto it = begin; it != end; ++it) {
			*this << *it;
		}
	}

	/**
	 * @brief Allows for writing a provided byte value a specified number of times to
	 * the stream.
	 * 
	 * @param The byte value that will fill the specified number of bytes.
	 */
	template<size_type size>
	constexpr void fill(const std::uint8_t value) requires writeable<buf_type> {
		const auto filled = generate_filled<size>(value);
		write(filled.data(), filled.size());
	}

	/*** Read ***/

	void deserialise(auto& object) {
		stream_read_adaptor adaptor(*this);
		object.serialise(adaptor);
	}

	binary_stream& operator>>(has_serialise<stream_read_adaptor<binary_stream>> auto& data)
	requires writeable<buf_type> {
		deserialise(data);
		return *this;
	}

	binary_stream& operator>>(prefixed<std::string> adaptor) {
		std::uint32_t size = 0;
		*this >> endian::le(size);

		if(state_ != stream_state::ok) {
			return *this;
		}

		STREAM_READ_BOUNDS_ENFORCE(size, *this);

		adaptor->resize_and_overwrite(size, [&](char* strbuf, std::size_t size) {
			buffer_.read(strbuf, size);
			return size;
		});

		return *this;
	}

	binary_stream& operator>>(prefixed<std::string_view> adaptor) {
		std::uint32_t size = 0;
		*this >> endian::le(size);

		if(state_ != stream_state::ok) {
			return *this;
		}

		adaptor.str = std::string_view { span<char>(size) };
		return *this;
	}
	
	binary_stream& operator>>(prefixed_varint<std::string> adaptor) {
		const auto size = varint_decode<size_type>(*this);

		// if an error was triggered during decode
		if(state_ != stream_state::ok) {
			return *this;
		}

		STREAM_READ_BOUNDS_ENFORCE(size, *this);

		adaptor->resize_and_overwrite(size, [&](char* strbuf, std::size_t size) {
			buffer_.read(strbuf, size);
			return size;
		});

		return *this;
	}

	binary_stream& operator>>(prefixed_varint<std::string_view> adaptor) {
		const auto size = varint_decode<size_type>(*this);

		// if an error was triggered during decode
		if(state_ != stream_state::ok) {
			return *this;
		}
		
		adaptor.str = std::string_view { span<char>(size) };
		return *this;
	}

	binary_stream& operator>>(null_terminated<std::string> adaptor) {
		auto pos = buffer_.find_first_of(value_type(0));

		if(pos == buf_type::npos) {
			adaptor->clear();
			return *this;
		}

		STREAM_READ_BOUNDS_ENFORCE(pos + 1, *this); // include null terminator

		adaptor->resize_and_overwrite(pos, [&](char* strbuf, std::size_t size) {
			buffer_.read(strbuf, pos);
			return size;
		});

		buffer_.skip(1); // skip null terminator
		return *this;
	}

	binary_stream& operator>>(null_terminated<std::string_view> adaptor) {
		adaptor.str = view();
		return *this;
	}

	binary_stream& operator >>(std::string_view& data) {
		return (*this >> prefixed(data));
	}

	binary_stream& operator >>(std::string& data) {
		return (*this >> prefixed(data));
	}

	binary_stream& operator>>(has_shr_override<binary_stream> auto&& data) {
		return data.operator>>(*this);
	}

	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	binary_stream& operator>>(endian_func adaptor) {
		SAFE_READ(&adaptor.value, sizeof(adaptor.value), *this);
		adaptor.value = adaptor.from();
		return *this;
	}

	binary_stream& operator>>(arithmetic auto& data) {
		SAFE_READ(&data, sizeof(data), *this);
		endian::storage_out(data, byte_order);
		return *this;
	}

	template<pod T>
	requires (!has_shr_override<T, binary_stream> && !arithmetic<T>)
	binary_stream& operator>>(T& data) {
		SAFE_READ(&data, sizeof(data), *this);
		return *this;
	}

	/**
	 * @brief Read an arithmetic type from the stream.
	 * 
	 * @return The destination for the read value.
	 */
	void get(arithmetic auto& dest) {
		SAFE_READ(&dest, sizeof(dest), void());
	}

	/**
	 * @brief Read an arithmetic type from the stream.
	 * 
	 * @return The arithmetic value.
	 */
	template<arithmetic T>
	T get() {
		T t{};
		SAFE_READ(&t, sizeof(T), t);
		return t;
	}

	/**
	 * @brief Read an arithmetic type from the stream, allowing for endian
	 * conversion.
	 * 
	 * @param The destination for the read value.
	 */
	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	void get(endian_func& adaptor) {
		SAFE_READ(&adaptor.value, sizeof(adaptor), void());
		adaptor.value = adaptor.from();
	}

	/**
	 * @brief Read an arithmetic type from the stream, allowing for endian
	 * conversion.
	 * 
	 * @return The arithmetic value.
	 */
	template<arithmetic T, endian::conversion conversion>
	T get() {
		T t{};
		SAFE_READ(&t, sizeof(T), t);
		return endian::convert<conversion>(t);
	}


	/**
	 * @brief Reads a string from the stream.
	 * 
	 * @param dest The destination string.
	 */
	void get(std::string& dest) {
		*this >> dest;
	}

	/**
	 * @brief Reads a fixed-length string from the stream.
	 * 
	 * @param dest The destination string.
	 * @param count The number of bytes to be read.
	 */
	void get(std::string& dest, size_type size) {
		STREAM_READ_BOUNDS_ENFORCE(size, void());

		dest.resize_and_overwrite(size, [&](char* strbuf, size_type len) {
			buffer_.read(strbuf, len);
			return len;
		});
	}

	/**
	 * @brief Read data from the stream into the provided destination argument.
	 * 
	 * @param dest The destination buffer.
	 * @param count The number of bytes to be read into the destination.
	 */
	template<typename T>
	void get(T* dest, size_type count) {
		assert(dest);
		const auto read_size = count * sizeof(T);
		SAFE_READ(dest, read_size, void());
	}

	/**
	 * @brief Read data from the stream to the destination represented by the iterators.
	 * 
	 * @param begin The beginning iterator.
	 * @param end The end iterator.
	 */
	template<typename It>
	void get(It begin, const It end) {
		for(; begin != end; ++begin) {
			*this >> *begin;
		}
	}

	/**
	 * @brief Read data from the stream into the provided destination argument.
	 * 
	 * @param dest A contiguous range into which the data should be read.
	 */
	template<std::ranges::contiguous_range range>
	void get(range& dest) {
		const auto read_size = dest.size() * sizeof(typename range::value_type);
		SAFE_READ(dest.data(), read_size, void());
	}

	/**
	 * @brief Skip over count bytes
	 *
	 * Skips over a number of bytes from the stream. This should be used
	 * if the stream holds data that you don't care about but don't want
	 * to have to read it to another buffer to move beyond it.
	 * 
	 * @param length The number of bytes to skip.
	 */
	void skip(const size_type count) {
		STREAM_READ_BOUNDS_ENFORCE(count, void());
		buffer_.skip(count);
	}

	/**
	 * @brief Provides a string_view over the stream's data, up to the terminator value.
	 * 
	 * @param terminator An optional terminating/sentinel value.
	 * 
	 * @return A string view over data up to the provided terminator.
	 * An empty string_view if a terminator is not found
	 */
	std::string_view view(value_type terminator = value_type(0)) requires contiguous<buf_type> {
		const auto pos = buffer_.find_first_of(terminator);

		if(pos == buf_type::npos) {
			return {};
		}

		std::string_view view { reinterpret_cast<char*>(buffer_.read_ptr()), pos };

		// no need to enforce bounds, we know there's enough data
		buffer_.skip(pos + 1);
		total_read_ += (pos + 1);
		return view;
	}

	/**
	 * @brief Provides a span over the specified number of elements in the stream.
	 * 
	 * @param count The number of elements the span will provide a view over.
	 * 
	 * @return A span representing a view over the requested number of elements
	 * in the stream.
	 * 
	 * @note The stream will error if the stream does not contain the requested amount of data.
	 */
	template<typename out_type = value_type>
	std::span<out_type> span(size_type count) requires contiguous<buf_type> {
		std::span view { reinterpret_cast<out_type*>(buffer_.read_ptr()), count };
		skip(sizeof(out_type) * count);
		return (state_ == stream_state::ok? view : std::span<out_type>());
	}

	/**  Misc functions **/

	/**
	 * @brief Determine whether the adaptor supports write seeking.
	 * 
	 * This is determined at compile-time and does not need to checked at
	 * run-time.
	 * 
	 * @return True if write seeking is supported, otherwise false.
	 */
	constexpr static bool can_write_seek() requires writeable<buf_type> {
		return std::is_same_v<seeking, supported>;
	}

	/**
	 * @brief Performs write seeking within the stream.
	 * 
	 * @param direction Specify whether to seek in a given direction or to absolute seek.
	 * @param offset The offset relative to the seek direction or the absolute value
	 * when using absolute seeking.
	 */
	void write_seek(const stream_seek direction, const offset_type offset)
		requires(seekable<buf_type> && writeable<buf_type>) {
		if(direction == stream_seek::sk_stream_absolute) {
			if(offset >= total_write_) {
				buffer_.write_seek(buffer_seek::sk_forward, offset - total_write_);
			} else {
				buffer_.write_seek(buffer_seek::sk_backward, total_write_ - offset);
			}

			total_write_ = offset;
		} else {
			buffer_.write_seek(static_cast<buffer_seek>(direction), offset);
		}
	}

	/**
	 * @brief Returns the size of the stream.
	 * 
	 * @return The number of bytes of data available to read within the stream.
	 */
	size_type size() const {
		return buffer_.size();
	}

	/**
	 * @brief Whether the stream is empty.
	 * 
	 * @return Returns true if the stream is empty (has no data to be read).
	 */
	[[nodiscard]]
	bool empty() const {
		return buffer_.empty();
	}

	/**
	 * @return The total number of bytes written to the stream.
	 */
	size_type total_write() const requires writeable<buf_type> {
		return total_write_;
	}

	/**
	 * @return Pointer to stream's underlying buffer.
	 */
	const buf_type* buffer() const {
		return &buffer_;
	}

	/**
	 * @return Pointer to stream's underlying buffer.
	 */
	buf_type* buffer() {
		return &buffer_;
	}

	/**
	 * @return The stream's state.
	 */
	stream_state state() const {
		return state_;
	}

	/**
	 * @return The total number of bytes read from the stream.
	 */
	size_type total_read() const {
		return total_read_;
	}

	/**
	 * @return If provided to the constructor, the upper limit on how much data
	 * can be read from this stream before an error is triggers.
	 */
	size_type read_limit() const {
		return read_limit_;
	}

	/**
	 * @brief Determine the maximum number of bytes that can be
	 * safely read from this stream.
	 * 
	 * The value returned may be lower than the amount of data
	 * available in the buffer if a read limit was set during
	 * the stream's construction.
	 * 
	 * @return The number of bytes available for reading.
	 */
	size_type read_max() const {
		if(read_limit_) {
			return read_limit_ - total_read_;
		} else {
			return buffer_.size();
		}
	}

	/**
	 * @brief Determine whether the stream is in an error state.
	 * 
	 * @return True if no errors occurred on this stream.
	 */
	bool good() const {
		return state_ == stream_state::ok;
	}

	/**
	 * @brief Clears the reset state of the stream if an error has occurred.
	 */
	void clear_error_state() {
		state_ = stream_state::ok;
	}

	operator bool() const {
		return good();
	}

	/**
	 * @brief Set the stream to an error state.
	 */
	void set_error_state() {
		state_ = stream_state::user_defined_err;
	}
};

} // hexi

// #include <hexi/buffer_adaptor.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/shared.h>

// #include <hexi/concepts.h>

// #include <hexi/exception.h>

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

	buffer_adaptor(buf_type& buffer, init_empty_t)
		: buffer_(buffer),
		  read_(0),
		  write_(0) {}

	buffer_adaptor(buffer_adaptor&& rhs) = delete;
	buffer_adaptor& operator=(buffer_adaptor&&) = delete;
	buffer_adaptor& operator=(const buffer_adaptor&) = delete;
	buffer_adaptor(const buffer_adaptor&) = delete;

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 */
	template<typename T>
	void read(T* destination) {
		read(destination, sizeof(T));
	}

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to read into the buffer.
	 */
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

	/**
	 * @brief Copies a number of bytes to the provided buffer but without advancing
	 * the read cursor.
	 * 
	 * @param destination The buffer to copy the data to.
	 */
	template<typename T>
	void copy(T* destination) const {
		copy(destination, sizeof(T));
	}

	/**
	 * @brief Copies a number of bytes to the provided buffer but without advancing
	 * the read cursor.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to copy.
	 */
	void copy(void* destination, size_type length) const {
		assert(destination && !region_overlap(buffer_.data(), buffer_.size(), destination, length));
		std::memcpy(destination, read_ptr(), length);
	}

	/**
	 * @brief Skip over length bytes.
	 * 
	 * Skips over a number of bytes from the container. This should be used
	 * if the container holds data that you don't care about but don't want
	 * to have to read it to another buffer to move beyond it.
	 * 
	 * @param length The number of bytes to skip.
	 */
	void skip(size_type length) {
		read_ += length;

		if constexpr(space_optimise) {
			if(read_ == write_) {
				read_ = write_ = 0;
			}
		}
	}

	/**
	 * @brief Write data to the container.
	 * 
	 * @param source Pointer to the data to be written.
	 */
	void write(const auto& source) {
		write(&source, sizeof(source));
	}

	/**
	 * @brief Write provided data to the container.
	 * 
	 * @param source Pointer to the data to be written.
	 * @param length Number of bytes to write from the source.
	 */
	void write(const void* source, size_type length) {
		assert(source && !region_overlap(source, length, buffer_.data(), buffer_.size()));
		const auto min_req_size = write_ + length;

		if(buffer_.size() < min_req_size) {
			if constexpr(has_resize_overwrite<buf_type>) {
				buffer_.resize_and_overwrite(min_req_size, [](char*, size_type size) {
					return size;
				});
			} else if constexpr(has_resize<buf_type>) {
				buffer_.resize(min_req_size);
			} else {
				HEXI_THROW(buffer_overflow(length, write_, free()));
			}
		}

		std::memcpy(write_ptr(), source, length);
		write_ += length;
	}

	/**
	 * @brief Attempts to locate the provided value within the container.
	 * 
	 * @param value The value to locate.
	 * 
	 * @return The position of value or npos if not found.
	 */
	size_type find_first_of(value_type val) const {
		const auto data = read_ptr();

		for(size_type i = 0, j = size(); i < j; ++i) {
			if(data[i] == val) {
				return i;
			}
		}

		return npos;
	}
	
	/**
	 * @brief Returns the size of the container.
	 * 
	 * @return The number of bytes of data available to read within the container.
	 */
	size_type size() const {
		return buffer_.size() - read_;
	}

	/**
	 * @brief Whether the container is empty.
	 * 
	 * @return Returns true if the container is empty (has no data to be read).
	 */
	[[nodiscard]]
	bool empty() const {
		return read_ == write_;
	}

	/**
	 * @brief Retrieves a reference to the specified index within the container.
	 * 
	 * @param index The index within the container.
	 * 
	 * @return A reference to the value at the specified index.
	 */
	value_type& operator[](const size_type index) {
		return read_ptr()[index];
	}

	/**
	 * @brief Retrieves a reference to the specified index within the container.
	 * 
	 * @param index The index within the container.
	 * 
	 * @return A reference to the value at the specified index.
	 */
	const value_type& operator[](const size_type index) const {
		return read_ptr()[index];
	}

	/**
	 * @brief Determine whether the adaptor supports write seeking.
	 * 
	 * This is determined at compile-time and does not need to checked at
	 * run-time.
	 * 
	 * @return True if write seeking is supported, otherwise false.
	 */
	constexpr static bool can_write_seek() {
		return std::is_same_v<seeking, supported>;
	}

	/**
	 * @brief Performs write seeking within the container.
	 * 
	 * @param direction Specify whether to seek in a given direction or to absolute seek.
	 * @param offset The offset relative to the seek direction or the absolute value
	 * when using absolute seeking.
	 */
	void write_seek(const buffer_seek direction, const offset_type offset) {
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

	/**
	 * @return Pointer to the data available for reading.
	 */
	auto read_ptr() const {
		return buffer_.data() + read_;
	}

	/**
	 * @return Pointer to the data available for reading.
	 */
	auto read_ptr() {
		return buffer_.data() + read_;
	}

	/**
	 * @return Pointer to the location within the buffer where the next write
	 * will be made.
	 */
	auto write_ptr() const  {
		return buffer_.data() + write_;
	}

	/**
	 * @return Pointer to the location within the buffer where the next write
	 * will be made.
	 */
	auto write_ptr() {
		return buffer_.data() + write_;
	}

	/**
	 * @return Pointer to the data available for reading.
	 */
	auto data() const {
		return buffer_.data() + read_;
	}

	/**
	 * @return Pointer to the data available for reading.
	 */
	auto data() {
		return buffer_.data() + read_;
	}

	/**
	 * @return Pointer to the underlying storage.
	 */
	auto storage() const {
		return buffer_.data();
	}

	/**
	 * @return Pointer to the underlying storage.
	 */
	auto storage() {
		return buffer_.data();
	}

	/**
	 * @brief Advances the write cursor.
	 * 
	 * @param size The number of bytes by which to advance the write cursor.
	 */
	void advance_write(size_type bytes) {
		assert(buffer_.size() >= (write_ + bytes));
		write_ += bytes;
	}

	auto free() const {
		return buffer_.size() - write_;
	}

	void clear() {
		read_ = write_ = 0;
	}
};

} // hexi

// #include <hexi/buffer_sequence.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#if defined HEXI_WITH_ASIO || defined HEXI_WITH_BOOST_ASIO

#ifdef HEXI_WITH_ASIO
#include <asio/buffer.hpp>
#elif defined HEXI_WITH_BOOST_ASIO
#include <boost/asio/buffer.hpp>
#endif

#ifdef HEXI_BUFFER_DEBUG
#include <span>
#endif

namespace hexi {

#ifdef HEXI_WITH_BOOST_ASIO
namespace asio = boost::asio;
#endif

template<typename buffer_type>
class buffer_sequence {
	const buffer_type& buffer_;

public:
	buffer_sequence(const buffer_type& buffer)
		: buffer_(buffer) { }

	class const_iterator {
		using node = typename buffer_type::node_type;

	public:
		const_iterator(const buffer_type& buffer, const node* curr_node)
			: buffer_(buffer),
			  curr_node_(curr_node) {}

		const_iterator& operator++() {
			curr_node_ = curr_node_->next;
			return *this;
		}

		const_iterator operator++(int) {
			const_iterator current(*this);
			curr_node_ = curr_node_->next;
			return current;
		}

		asio::const_buffer operator*() const {
			const auto buffer = buffer_.buffer_from_node(curr_node_);
			return buffer->read_data();
		}

		bool operator==(const const_iterator& rhs) const {
			return curr_node_ == rhs.curr_node_;
		}

		bool operator!=(const const_iterator& rhs) const {
			return curr_node_ != rhs.curr_node_;
		}

		const_iterator& operator=(const_iterator&) = delete;

	#ifdef HEXI_BUFFER_DEBUG
		std::span<const char> get_buffer() {
			auto buffer = buffer_.buffer_from_node(curr_node_);
			return {
				reinterpret_cast<const char*>(buffer->read_ptr()), buffer->size()
			};
		}
	#endif

	private:
		const buffer_type& buffer_;
		const node* curr_node_;
	};

const_iterator begin() const {
	return const_iterator(buffer_, buffer_.root_.next);
}

const_iterator end() const {
	return const_iterator(buffer_, &buffer_.root_);
}

friend class const_iterator;
};

} // hexi

#endif // #if defined HEXI_WITH_ASIO || defined HEXI_WITH_BOOST_ASIO
// #include <hexi/concepts.h>

// #include <hexi/dynamic_buffer.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/pmc/buffer.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/pmc/buffer_read.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/pmc/buffer_base.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



#include <cstddef>

namespace hexi::pmc {

class buffer_base {
public:
	virtual std::size_t size() const = 0;
	virtual bool empty() const = 0;
	virtual ~buffer_base() = default;
};

} // pmc, hexi

#include <cstddef>

namespace hexi::pmc {

class buffer_read : virtual public buffer_base {
public:
	using value_type = std::byte;

	static constexpr auto npos { static_cast<std::size_t>(-1) };

	virtual ~buffer_read() = default;
	virtual void read(void* destination, std::size_t length) = 0;
	virtual void copy(void* destination, std::size_t length) const = 0;
	virtual	void skip(std::size_t length) = 0;
	virtual const std::byte& operator[](const std::size_t index) const = 0;
	virtual std::size_t find_first_of(std::byte val) const = 0;
};

} // pmc, hexi

// #include <hexi/pmc/buffer_write.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/pmc/buffer_base.h>

// #include <hexi/shared.h>

#include <cstddef>

namespace hexi::pmc {

class buffer_write : virtual public buffer_base {
public:
	using value_type = std::byte;

	virtual ~buffer_write() = default;
	virtual void write(const void* source, std::size_t length) = 0;
	virtual void reserve(std::size_t length) = 0;
	virtual bool can_write_seek() const = 0;
	virtual void write_seek(buffer_seek direction, std::size_t offset) = 0;
};

} // pmc, hexi


namespace hexi::pmc {

class buffer : public buffer_read, public buffer_write {
public:
	using value_type = std::byte;

	using buffer_read::operator[];

	virtual std::byte& operator[](const std::size_t index) = 0;
	virtual ~buffer() = default;
};

} // pmc, hexi

// #include <hexi/shared.h>

// #include <hexi/allocators/default_allocator.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



#include <utility>

namespace hexi {

template<typename T>
struct default_allocator final {
	template<typename ...Args>
	[[nodiscard]] inline T* allocate(Args&&... args) const {
		return new T(std::forward<Args>(args)...);
	}

	inline void deallocate(T* t) const {
		delete t;
	}
};

} // hexi

// #include <hexi/detail/intrusive_storage.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



#if _MSC_VER
#pragma warning(disable : 4996)
#endif

// #include <hexi/shared.h>

// #include <hexi/concepts.h>

#include <array>
#include <concepts>
#include <span>
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
	using offset_type = std::remove_const_t<decltype(block_size)>;

	offset_type read_offset = 0;
	offset_type write_offset = 0;
	intrusive_node node {};
	std::array<value_type, block_size> storage;

	/**
	 * @brief Clear the container.
	 * 
	 * Resets the read and write offsets, does not explicitly
	 * erase the data contained within the buffer but should
	 * be treated as such.
	 */
	void clear() {
		read_offset = 0;
		write_offset = 0;
	}

	/**
	 * @brief Write provided data to the container.
	 * 
	 * If the container size is lower than requested number of bytes,
	 * the request will be capped at the number of bytes available.
	 * 
	 * @param source Pointer to the data to be written.
	 * @param length Number of bytes to write from the source.
	 * 
	 * @return The number of bytes copied, which may be less than requested.
	 */
	std::size_t write(const auto source, std::size_t length) {
		assert(!region_overlap(source, length, storage.data(), storage.size()));
		std::size_t write_len = block_size - write_offset;

		if(write_len > length) {
			write_len = length;
		}

		std::memcpy(storage.data() + write_offset, source, write_len);
		write_offset += static_cast<offset_type>(write_len);
		return write_len;
	}

	/**
	 * @brief Copies a number of bytes to the provided buffer but without
	 * advancing the read cursor.
	 * 
	 * If the container size is lower than requested number of bytes,
	 * the request will be capped at the number of bytes available.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to copy.
	 * 
	 * @return The number of bytes copied, which may be less than requested.
	 */
	std::size_t copy(auto destination, const std::size_t length) const {
		assert(!region_overlap(storage.data(), storage.size(), destination, length));
		std::size_t read_len = block_size - read_offset;

		if(read_len > length) {
			read_len = length;
		}

		std::memcpy(destination, storage.data() + read_offset, read_len);
		return read_len;
	}

	/**
	* @brief Reads a number of bytes to the provided buffer.
	* 
	* If the container size is lower than requested number of bytes,
	* the request will be capped at the number of bytes available.
	* 
	* @param length The number of bytes to skip.
	* @param destination The buffer to copy the data to.
	* @param allow_optimise Whether the buffer can reuse its space where possible.
	* 
	* @return The number of bytes read, which may be less than requested.
	*/
	std::size_t read(auto destination, const std::size_t length, const bool allow_optimise = false) {
		std::size_t read_len = copy(destination, length);
		read_offset += static_cast<offset_type>(read_len);

		if(read_offset == write_offset && allow_optimise) {
			clear();
		}

		return read_len;
	}

	/**
	* @brief Skip over requested number of bytes.
	*
	* Skips over a number of bytes in the container. This should be used
	* if the container holds data that you don't care about but don't want
	* to have to read it to another buffer to move beyond it.
	* 
	* If the container size is lower than requested number of bytes,
	* the request will be capped at the number of bytes available.
	* 
	* @param length The number of bytes to skip.
	* @param allow_optimise Whether the buffer can reuse its space where possible.
	* 
	* @return The number of bytes skipped, which may be less than requested.
	*/
	std::size_t skip(const std::size_t length, const bool allow_optimise = false) {
		std::size_t skip_len = block_size - read_offset;

		if(skip_len > length) {
			skip_len = length;
		}

		read_offset += static_cast<offset_type>(skip_len);

		if(read_offset == write_offset && allow_optimise) {
			clear();
		}

		return skip_len;
	}

	/**
	 * @brief Returns the size of the container.
	 * 
	 * @return The number of bytes available to read within the container.
	 */
	std::size_t size() const {
		return write_offset - read_offset;
	}

	/**
	 * @brief The amount of free space.
	 * 
	 * @return The number of bytes of free space within the container.
	 */
	std::size_t free() const {
		return block_size - write_offset;
	}

	/**
	 * @brief Performs write seeking within the container.
	 * 
	 * @param direction Specify whether to seek in a given direction or to absolute seek.
	 * @param offset The offset relative to the seek direction or the absolute value
	 * when using absolute seeking.
	 */
	void write_seek(const buffer_seek direction, const std::size_t offset) {
		switch(direction) {
			case buffer_seek::sk_absolute:
				write_offset = offset;
				break;
			case buffer_seek::sk_backward:
				write_offset -= static_cast<offset_type>(offset);
				break;
			case buffer_seek::sk_forward:
				write_offset += static_cast<offset_type>(offset);
				break;
		}
	}

	/**
	 * @brief Advances the write cursor.
	 * 
	 * If the requested number of bytes exceeds the free space, the
	 * request will be capped at the amount of free space.
	 * 
	 * @param size The number of bytes by which to advance the write cursor.
	 * 
	 * @return The number of bytes the write cursor was advanced by, which
	 * may be less than requested.
	 */
	std::size_t advance_write(std::size_t size) {
		const auto remaining = free();

		if(remaining < size) {
			size = remaining;
		}

		write_offset += static_cast<offset_type>(size);
		return size;
	}

	/**
	 * @brief Retrieve a pointer to the readable portion of the buffer.
	 * 
	 * @return Pointer to the readable portion of the buffer.
	 */
	const value_type* read_ptr() const {
		return storage.data() + read_offset;
	}

	/**
	 * @brief Retrieve a pointer to the readable portion of the buffer.
	 * 
	 * @return Pointer to the readable portion of the buffer.
	 */
	value_type* read_ptr() {
		return storage.data() + read_offset;
	}

	/**
	 * @brief Retrieve a pointer to the writeable portion of the buffer.
	 * 
	 * @return Pointer to the writeable portion of the buffer.
	 */
	const value_type* write_ptr () const {
		return storage.data() + write_offset;
	}

	/**
	 * @brief Retrieve a pointer to the writeable portion of the buffer.
	 * 
	 * @return Pointer to the writeable portion of the buffer.
	 */
	value_type* write_ptr() {
		return storage.data() + write_offset;
	}

	/**
	 * @brief Retrieve a span to the readable portion of the buffer.
	 * 
	 * @return Span to the readable portion of the buffer.
	 */
	std::span<const value_type> read_data() const {
		return { storage.data() + read_offset, size() };
	}

	/**
	 * @brief Retrieve a span to the readable portion of the buffer.
	 * 
	 * @return Span to the readable portion of the buffer.
	 */
	std::span<value_type> read_data() {
		return { storage.data() + read_offset, size() } ;
	}

	/**
	 * @brief Retrieve a span to the writeable portion of the buffer.
	 * 
	 * @return Span to the writeable portion of the buffer.
	 */
	std::span<const value_type> write_data() const {
		return { storage.data() + write_offset, free() } ;
	}

	/**
	 * @brief Retrieve a span to the writeable portion of the buffer.
	 * 
	 * @return Span to the writeable portion of the buffer.
	 */
	std::span<value_type> write_data() {
		return { storage.data() + write_offset, free() } ;
	}

	/**
	 * @brief Retrieves a reference to the specified index within the container.
	 * 
	 * @param index The index within the container.
	 * 
	 * @return A reference to the value at the specified index.
	 */
	value_type& operator[](const std::size_t index) {
		return *(storage.data() + index);
	}

	/**
	 * @brief Retrieves a reference to the specified index within the container.
	 * 
	 * @param index The index within the container.
	 * 
	 * @return A reference to the value at the specified index.
	 */
	const value_type& operator[](const std::size_t index) const {
		return *(storage.data() + index);
	}
};

} // detail, hexi

#include <concepts>
#include <functional>
#include <memory>
#include <utility>
#ifdef HEXI_BUFFER_DEBUG
#include <algorithm>
#include <vector>
#endif
#include <cstddef>
#include <cstdint>
#include <cassert>

namespace hexi {

using namespace detail;

template<typename buffer_type>
class buffer_sequence;

template<decltype(auto) block_sz>
concept int_gt_zero = std::integral<decltype(block_sz)> && block_sz > 0;

template<decltype(auto) block_sz,
	byte_type storage_value_type = std::byte,
	typename allocator = default_allocator<detail::intrusive_storage<block_sz, storage_value_type>>
>
requires int_gt_zero<block_sz>
class dynamic_buffer final : public pmc::buffer {
public:
	using storage_type = intrusive_storage<block_sz, storage_value_type>;
	using value_type   = storage_value_type;
	using node_type    = intrusive_node;
	using size_type    = std::size_t;
	using offset_type  = std::size_t;
	using contiguous   = is_non_contiguous;
	using seeking      = supported;

	static constexpr auto npos { static_cast<size_type>(-1) };

	using unique_storage = std::unique_ptr<storage_type, std::function<void(storage_type*)>>;

private:
	intrusive_node root_;
	size_type size_;
	[[no_unique_address]] allocator allocator_;

	void link_tail_node(intrusive_node* node) {
		node->next = &root_;
		node->prev = root_.prev;
		root_.prev = root_.prev->next = node;
	}

	void unlink_node(intrusive_node* node) {
		node->next->prev = node->prev;
		node->prev->next = node->next;
	}

	inline storage_type* buffer_from_node(const intrusive_node* node) const {
		return reinterpret_cast<storage_type*>(std::uintptr_t(node)
			- offsetof(storage_type, node));
	}

	void move(dynamic_buffer& rhs) noexcept {
		if(this == &rhs) { // self-assignment
			return;
		}

		clear(); // clear our current blocks rather than swapping them

		size_ = rhs.size_;
		root_ = rhs.root_;
		root_.next->prev = &root_;
		root_.prev->next = &root_;
		rhs.size_ = 0;
		rhs.root_.next = &rhs.root_;
		rhs.root_.prev = &rhs.root_;
	}

	void copy(const dynamic_buffer& rhs) {
		if(this == &rhs) { // self-assignment
			return;
		}

		const intrusive_node* head = rhs.root_.next;
		root_.next = &root_;
		root_.prev = &root_;
		size_ = 0;

		while(head != &rhs.root_) {
			auto buffer = allocate();
			*buffer = *buffer_from_node(head);
			link_tail_node(&buffer->node);
			size_ += buffer->write_offset;
			head = head->next;
		}
	}
	
#ifdef HEXI_BUFFER_DEBUG
	void offset_buffers(std::vector<storage_type*>& buffers, size_type offset) {
		std::erase_if(buffers, [&](auto block) {
			if(block->size() > offset) {
				block->read_offset += offset;
				block->write_offset -= offset;
				return false;
			} else {
				return true;
			}
		});
	}
#endif

	value_type& byte_at_index(const size_type index) const {
		assert(index < size_ && "buffer subscript index out of range");

		auto head = root_.next;
		auto buffer = buffer_from_node(head);
		const auto offset_index = index + buffer->read_offset;
		const auto node_index = offset_index / block_sz;

		for(size_type i = 0; i < node_index; ++i) {
			head = head->next;
		}

		buffer = buffer_from_node(head);
		return (*buffer)[offset_index % block_sz];
	}

	size_type abs_seek_offset(size_type offset) {
		if(offset < size_) {
			return size_ - offset;
		} else if(offset > size_) {
			return offset - size_;
		} else {
			return 0;
		}
	}

	[[nodiscard]] storage_type* allocate() {
		return allocator_.allocate();
	}

	void deallocate(storage_type* buffer) {
		allocator_.deallocate(buffer);
	}

public:
	dynamic_buffer()
		: root_{ .next = &root_, .prev = &root_ },
		  size_(0) {}

	~dynamic_buffer() {
		clear();
	}

	dynamic_buffer& operator=(dynamic_buffer&& rhs) noexcept {
		move(rhs);
		return *this;
	}

	dynamic_buffer(dynamic_buffer&& rhs) noexcept {
		move(rhs);
	}

	dynamic_buffer(const dynamic_buffer& rhs) {
		copy(rhs);
	}

	dynamic_buffer& operator=(const dynamic_buffer& rhs) {
		clear();
		copy(rhs);
		return *this;
	}

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 */
	template<typename T>
	void read(T* destination) {
		read(destination, sizeof(T));
	}

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to read into the buffer.
	 */
	void read(void* destination, size_type length) override {
		assert(length <= size_ && "Chained buffer read too large!");
		size_type remaining = length;

		while(true) {
			auto buffer = buffer_from_node(root_.next);
			remaining -= buffer->read(
				static_cast<value_type*>(destination) + length - remaining, remaining,
				                         root_.next == root_.prev
			);

			if(remaining) [[unlikely]] {
				unlink_node(root_.next);
				deallocate(buffer);
			} else {
				break;
			}
		}

		size_ -= length;
	}

	/**
	 * @brief Copies a number of bytes to the provided buffer but without advancing
	 * the container's read cursor.
	 * 
	 * @param destination The buffer to copy the data to.
	 */
	template<typename T>
	void copy(T* destination) const {
		copy(destination, sizeof(T));
	}

	/**
	 * @brief Copies a number of bytes to the provided buffer but without advancing
	 * the container's read cursor.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to copy.
	 */
	void copy(void* destination, const size_type length) const override {
		assert(length <= size_ && "Chained buffer copy too large!");
		size_type remaining = length;
		auto head = root_.next;

		while(true) {
			const auto buffer = buffer_from_node(head);
			remaining -= buffer->copy(
				static_cast<value_type*>(destination) + length - remaining, remaining
			);

			if(remaining) [[unlikely]] {
				head = head->next;
			} else {
				break;
			}
		}
	}

#ifdef HEXI_BUFFER_DEBUG
	std::vector<storage_type*> fetch_buffers(const size_type length, const size_type offset = 0) {
		size_type total = length + offset;
		assert(total <= size_ && "Chained buffer fetch too large!");
		std::vector<storage_type*> buffers;
		auto head = root_.next;

		while(total) {
			auto buffer = buffer_from_node(head);
			size_type read_size = block_sz - buffer->read_offset;
			
			// guard against overflow - buffer may have more content than requested
			if(read_size > total) {
				read_size = total;
			}

			buffers.emplace_back(buffer);
			total -= read_size;
			head = head->next;
		}

		if(offset) {
			offset_buffers(buffers, offset);
		}

		return buffers;
	}
#endif

	/**
	 * @brief Skip the requested number of bytes.
	 *
	 * Skips over a number of bytes from the container. This should be used
	 * if the container holds data that you don't care about but don't want
	 * to have to read it to another buffer to move beyond it.
	 * 
	 * @param length The number of bytes to skip.
	 */
	void skip(const size_type length) override {
		assert(length <= size_ && "Chained buffer skip too large!");
		size_type remaining = length;

		while(true) {
			auto buffer = buffer_from_node(root_.next);
			remaining -= buffer->skip(remaining, root_.next == root_.prev);

			if(remaining) [[unlikely]] {
				unlink_node(root_.next);
				deallocate(buffer);
			} else {
				break;
			}
		}

		size_ -= length;
	}

	/**
	 * @brief Write data to the container.
	 * 
	 * @param source Pointer to the data to be written.
	 */
	void write(const auto& source) {
		write(&source, sizeof(source));
	}

	/**
	 * @brief Write provided data to the container.
	 * 
	 * @param source Pointer to the data to be written.
	 * @param length Number of bytes to write from the source.
	 */
	void write(const void* source, const size_type length) override {
		size_type remaining = length;
		intrusive_node* tail = root_.prev;

		do {
			storage_type* buffer;

			if(tail != &root_) [[likely]] {
				buffer = buffer_from_node(tail);
			} else {
				buffer = allocate();
				link_tail_node(&buffer->node);
				tail = root_.prev;
			}

			remaining -= buffer->write(
				static_cast<const value_type*>(source) + length - remaining, remaining
			);

			tail = tail->next;
		} while(remaining);

		size_ += length;
	}

	/**
	 * @brief Reserves a number of bytes within the container for future use.
	 * 
	 * @param length The number of bytes that the container should reserve.
	 */
	void reserve(const size_type length) override {
		size_type remaining = length;
		intrusive_node* tail = root_.prev;

		do {
			storage_type* buffer;

			if(tail == &root_) [[unlikely]] {
				buffer = allocate();
				link_tail_node(&buffer->node);
				tail = root_.prev;
			} else {
				buffer = buffer_from_node(tail);
			}

			remaining -= buffer->advance_write(remaining);
			tail = tail->next;
		} while(remaining);

		size_ += length;
	}

	/**
	 * @brief Returns the size of the container.
	 * 
	 * @return The number of bytes of data available to read within the container.
	 */
	size_type size() const override {
		return size_;
	}

	/**
	 * @brief Retrieves the last buffer in the container's list.
	 * 
	 * @return A reference to the value at the specified index.
	 * @note The container retains ownership over the buffer, so it must not
	 * be deallocated by the caller.
	 */
	storage_type* back() const {
		if(root_.prev == &root_) {
			return nullptr;
		}

		return buffer_from_node(root_.prev);
	}

	/**
	 * @brief Retrieves the buffer's block sized.
	 * 
	 * @param index The index within the buffer.
	 * @return A reference to the value at the specified index.
	 */
	storage_type* front() const {
		if(root_.next == &root_) {
			return nullptr;
		}

		return buffer_from_node(root_.next);
	}

	/**
	 * @brief Removes the first buffer from the container.
	 * 
	 * @return A pointer to the popped buffer.
	 * 
	 * @note Removing the buffer from the container also transfers ownership.
	 * Therefore, the caller assumes responsibility for deallocating the buffer
	 * when it is no longer required. Alternatively, it can be pushed back into
	 * the container to transfer ownership back.
	 */
	auto pop_front() {
		auto buffer = buffer_from_node(root_.next);
		size_ -= buffer->size();
		unlink_node(root_.next);
		return unique_storage(buffer, [&](auto ptr) {
			deallocate(ptr);
		});
	}

	/**
	 * @brief Attaches additional storage to the container.
	 * 
	 * @param buffer The block to add to the container.
	 * 
	 * @note Once pushed, the container is assumed to have ownership over the buffer.
	 * The buffer storage must have been allocated by the same allocator as the container.
	 */
	void push_back(storage_type* buffer) {
		link_tail_node(&buffer->node);
		size_ += buffer->write_offset;
	}

	/**
	 * @brief Advances the write cursor.
	 * 
	 * @param size The number of bytes by which the write cursor to advance the cursor.
	 */
	void advance_write(const size_type size) {
		auto buffer = buffer_from_node(root_.prev);
		const auto actual = buffer->advance_write(size);
		assert(size <= block_sz && actual <= size &&
		       "Attempted to advance write cursor out of bounds!");
		size_ += size;
	}

	/**
	 * @brief Determine whether the adaptor supports write seeking.
	 * 
	 * This is determined at compile-time and does not need to checked at
	 * run-time.
	 * 
	 * @return True if write seeking is supported, otherwise false.
	 */
	bool can_write_seek() const override {
		return std::is_same_v<seeking, supported>;
	}

	/**
	 * @brief Performs write seeking within the container.
	 * 
	 * @param direction Specify whether to seek in a given direction or to absolute seek.
	 * @param offset The offset relative to the seek direction or the absolute value
	 * when using absolute seeking.
	 */
	void write_seek(const buffer_seek direction, size_type offset) override {
		// nothing to do in this case
		if(direction == buffer_seek::sk_absolute && offset == size_) {
			return;
		}

		auto tail = root_.prev;

		switch(direction) {
			case buffer_seek::sk_backward:
				size_ -= offset;
				break;
			case buffer_seek::sk_forward:
				size_ += offset;
				break;
			case buffer_seek::sk_absolute:
				size_ = offset;
				offset = abs_seek_offset(offset);
				break;
		}

		const bool rewind
			= (direction == buffer_seek::sk_backward
			   || (direction == buffer_seek::sk_absolute && offset < size_));

		while(offset) {
			auto buffer = buffer_from_node(tail);
			const auto max_seek = rewind? buffer->size() : buffer->free();

			if(max_seek >= offset) {
				buffer->write_seek(direction, offset);
				offset = 0;
			} else {
				buffer->write_seek(direction, max_seek);
				offset -= max_seek;
				tail = rewind? tail->prev : tail->next;
			}
		}

		root_.prev = tail;
	}

	/**
	 * @brief Clears the container.
	 */
	void clear() {
		intrusive_node* head = root_.next;

		while(head != &root_) {
			auto next = head->next;
			deallocate(buffer_from_node(head));
			head = next;
		}

		root_.next = &root_;
		root_.prev = &root_;
		size_ = 0;
	}

	/**
	 * @brief Whether the container is empty.
	 * 
	 * @return Returns true if the container is empty (has no data to be read).
	 */
	[[nodiscard]]
	bool empty() const override {
		return !size_;
	}

	/**
	 * @brief Retrieves the container's block size.
	 * 
	 * @return The block size.
	 */
	consteval static size_type block_size() {
		return block_sz;
	}

	/**
	 * @brief Retrieves a reference to the specified index within the container.
	 * 
	 * @param index The index within the container.
	 * 
	 * @return A reference to the value at the specified index.
	 */
	value_type& operator[](const size_type index) override {
		return byte_at_index(index);
	}

	/**
	 * @brief Retrieves a reference to the specified index within the container.
	 * 
	 * @param index The index within the container.
	 * 
	 * @return A reference to the value at the specified index.
	 */
	const value_type& operator[](const size_type index) const override {
		return byte_at_index(index);
	}

	/**
	 * @brief Retrieves the number of allocated blocks currently being used by the container.
	 * 
	 * @return The number of allocated blocks.
	 */
	size_type block_count() const {
		auto node = &root_;
		size_type count = 0;

		// not calculating based on block size & size as it
		// wouldn't play nice with seeking or manual push/pop
		while(node->next != root_.prev->next) {
			++count;
			node = node->next;
		}

		return count;
	}

	/**
	 * @brief Attempts to locate the provided value within the container.
	 * 
	 * @param value The value to locate.
	 * 
	 * @return The position of value or npos if not found.
	 */
	size_type find_first_of(value_type value) const override {
		size_type index = 0;
		auto head = root_.next;

		while(head != &root_) {
			const auto buffer = buffer_from_node(head);
			const auto data = buffer->read_data();
			
			for(const auto& byte : data) {
				if(byte == value) {
					return index;
				}

				++index;
			}

			head = head->next;
		}

		return npos;
	}

	/**
	 * @brief Retrieves the container's allocator.
	 * 
	 * @return The memory allocator.
	 */
	auto& get_allocator() {
		return allocator_;
	}

	/**
	 * @brief Retrieves the container's allocator.
	 * 
	 * @return The memory allocator.
	 */
	auto& get_allocator() const {
		return allocator_;
	}

	template<typename buffer_type>
	friend class buffer_sequence;
};

} // hexi

// #include <hexi/dynamic_tls_buffer.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/dynamic_buffer.h>

// #include <hexi/allocators/tls_block_allocator.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/allocators/block_allocator.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



#include <array>
#include <memory>
#include <new>
#include <thread>
#include <type_traits>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstdint>

#ifndef NDEBUG
#define HEXI_DEBUG_ALLOCATORS
#endif

namespace hexi {

namespace detail {

struct free_block {
	free_block* next;
};

template<decltype(auto) size>
concept gt_zero = size > 0;

template<typename T, typename U>
concept sizeof_gte = sizeof(T) >= sizeof(U);

} // detail

using namespace detail;

struct no_validate_dealloc {};
struct validate_dealloc : no_validate_dealloc {};

/**
 * Basic fixed-size block stack allocator that preallocates a slab of memory
 * capable of holding a compile-time determined number of elements.
 * When constructed, a linked list of chunks is built within the slab and
 * each allocation request will take the head node. Since the allocations
 * are fixed-size, the list does not need to be traversed for a suitable
 * size. Deallocations place the chunk as the new head (LIFO).
 *
 * If the preallocated slab runs out of chunks, it will fall back to using the
 * system allocator rather than allocating additional slabs. This means sizing
 * the initial allocation correctly is important for maximum performance, so
 * it's better to be pessimistic. This is a server application and RAM is cheap. :)
 *
 * PagePolicy: 'lock' requests that the OS does not page out the memory slab to disk.
 *
 * ThreadPolicy: 'same_thread' triggers an assert if an allocated object
 * is deallocated from a different thread. Used by the TLS allocator, since
 * implementing the functionality there is messier (and slower).
 */
template<typename _ty, 
	std::size_t _elements,
	std::derived_from<no_validate_dealloc> ValidatePolicy = no_validate_dealloc>
requires gt_zero<_elements> && sizeof_gte<_ty, free_block>
class block_allocator {
	using tid_type = std::conditional_t<
		std::is_same_v<ValidatePolicy, validate_dealloc>, std::thread::id, std::monostate
	>;

	struct mem_block {
		_ty obj;

		struct {
			[[no_unique_address]] tid_type thread_id;
			bool using_new;
		} meta;
	};

	static constexpr auto block_size = sizeof(mem_block);

	free_block* head_ = nullptr;
	[[no_unique_address]] tid_type thread_id_;
	std::array<char, block_size * _elements> storage_;

	void initialise_free_list() {
		auto storage = storage_.data();

		for(std::size_t i = 0; i < _elements; ++i) {
			auto block = reinterpret_cast<free_block*>(storage + (block_size * i));
			push(block);
		}
	}

	inline void push(free_block* block) {
		assert(block);
		block->next = head_;
		head_ = block;
	}

	[[nodiscard]] inline free_block* pop() {
		if(!head_) {
			return nullptr;
		}

		auto block = head_;
		head_ = block->next;
		return block;
	}

public:
#ifdef HEXI_DEBUG_ALLOCATORS
	std::size_t storage_active_count = 0;
	std::size_t new_active_count = 0;
	std::size_t active_count = 0;
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
#endif

	block_allocator() requires std::same_as<ValidatePolicy, validate_dealloc>
		: thread_id_(std::this_thread::get_id()) {
		initialise_free_list();
	}

	block_allocator() {
		initialise_free_list();
	}

	template<typename ...Args>
	[[nodiscard]] inline _ty* allocate(Args&&... args) {
		auto block = reinterpret_cast<mem_block*>(pop());

		if(block) [[likely]] {
#ifdef HEXI_DEBUG_ALLOCATORS
			++storage_active_count;
#endif
			block->meta.using_new = false;
		} else {
#ifdef HEXI_DEBUG_ALLOCATORS
			++new_active_count;
#endif
			block = new mem_block;
			block->meta.using_new = true;
		}

		if constexpr(std::is_same_v<ValidatePolicy, validate_dealloc>) {
			block->meta.thread_id = thread_id_;
		}

#ifdef HEXI_DEBUG_ALLOCATORS
		++total_allocs;
		++active_count;
#endif
		return new (&block->obj) _ty(std::forward<Args>(args)...);
	}

	inline void deallocate(_ty* t) {
		assert(t);
		auto block = reinterpret_cast<mem_block*>(t);

		if constexpr(std::is_same_v<ValidatePolicy, validate_dealloc>) {
			assert(block->meta.thread_id == thread_id_
				&& "thread policy violation or clobbered block");
		}

		if(block->meta.using_new) [[unlikely]] {
#ifdef HEXI_DEBUG_ALLOCATORS
			--new_active_count;
#endif
			t->~_ty();
			delete block;
		} else {
#ifdef HEXI_DEBUG_ALLOCATORS
			--storage_active_count;
#endif
			t->~_ty();
			push(reinterpret_cast<free_block*>(t));
		}

#ifdef HEXI_DEBUG_ALLOCATORS
		++total_deallocs;
		--active_count;
#endif
	}

	~block_allocator() {
#ifdef HEXI_DEBUG_ALLOCATORS
		assert(active_count == 0);
#endif
	}
};

} // hexi

#include <type_traits>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstdint>

#ifndef NDEBUG
#define HEXI_DEBUG_ALLOCATORS
#endif

namespace hexi {

struct safe_entrant {};
struct no_ref_counting {};

struct unsafe_entrant : safe_entrant {};
struct ref_counting : no_ref_counting {};

template<typename _ty,
	std::size_t _elements,
	std::derived_from<no_ref_counting> ref_count_policy = no_ref_counting,
	std::derived_from<safe_entrant> entrant_policy = safe_entrant
>
class tls_block_allocator final {
	using allocator_type = block_allocator<_ty, _elements>;

	using ref_count = std::conditional_t<
		std::is_same_v<ref_count_policy, ref_counting>, int, std::monostate
	>;

	using tls_handle_cache = std::conditional_t<
		std::is_same_v<entrant_policy, unsafe_entrant>, allocator_type*, std::monostate
	>;

	static inline thread_local std::unique_ptr<allocator_type> allocator_;
	static inline thread_local ref_count ref_count_{};

	[[no_unique_address]] tls_handle_cache cached_handle_{};

	// Compiler will optimise calls to this out when using unsafe_entrant
	inline void initialise() {
		if constexpr(std::is_same_v<entrant_policy, safe_entrant>) {
			if(!allocator_) {
				allocator_ = std::make_unique<allocator_type>();
			}
		}
	}

	inline allocator_type* allocator_handle() {
		if constexpr(std::is_same_v<entrant_policy, unsafe_entrant>) {
			return cached_handle_;
		} else {
			return allocator_.get();
		}
	}

public:
#ifdef HEXI_DEBUG_ALLOCATORS
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
	std::size_t active_allocs = 0;
#endif

	tls_block_allocator() {
		thread_enter();
	}

	/*
	 * When used in conjunction with unsafe_entrant, allows the owning object
	 * to be executed on another thread without paying for checks on every
	 * allocation
	 */
	inline void thread_enter() {
		if(!allocator_) {
			allocator_ = std::make_unique<allocator_type>();
		}

		if constexpr(std::is_same_v<entrant_policy, unsafe_entrant>) {
			cached_handle_ = allocator_.get();
		}

		if constexpr(std::is_same_v<ref_count_policy, ref_counting>) {
			++ref_count_;
		}
	}

	inline void thread_exit() {
		if constexpr(std::is_same_v<ref_count_policy, ref_counting>) {
			assert(ref_count_);

			--ref_count_;

			if(ref_count_ == 0) {
				allocator_.reset();
			}
		}
	}

	template<typename ...Args>
	[[nodiscard]] inline _ty* allocate(Args&&... args) {
		/*
		 * When safe_entrant is set, need to do this here & in ctor unless
		 * we can be 100% sure that any object using the allocator is created
		 * on the same thread that ends up using it.
		 */
		initialise();

#ifdef HEXI_DEBUG_ALLOCATORS
		++total_allocs;
		++active_allocs;
#endif
		return allocator_handle()->allocate(std::forward<Args>(args)...);
	}

	inline void deallocate(_ty* t) {
		assert(t);

#ifdef HEXI_DEBUG_ALLOCATORS
		++total_deallocs;
		--active_allocs;
#endif
		allocator_handle()->deallocate(t);
	}

#ifdef HEXI_DEBUG_ALLOCATORS
	auto allocator() {
		initialise();
		return allocator_handle();
	}
#endif

	~tls_block_allocator() {
		thread_exit();

#ifdef HEXI_DEBUG_ALLOCATORS
		assert(active_allocs == 0);
#endif
	}
};

} // hexi

#include <cstddef>

namespace hexi {

// dynamic_buffer backed by thread-local storage, meaning every
// instance on the same thread shares the same underlying memory.
// As a rule of thumb, an instance should never be touched by any thread
// other than the one on which it was created, not even if synchronised
// ... unless you're positive it won't result in the allocator being called.
// 
// Minimum memory usage is intrusive_storage<block_size> * count.
// Additional blocks are not added if the original is exhausted ('colony' structure),
// so the allocator will fall back to the system allocator instead.
//
// Pros: extremely fast allocation/deallocation for many instances per thread
// Cons: everything else.
// 
// TL;DR Do not use unless you know what you're doing.
template<decltype(auto) block_size,
	std::size_t count,
	typename ref_count_policy = no_ref_counting,
	typename entrant_policy = safe_entrant,
	typename storage_type = std::byte>
using dynamic_tls_buffer = dynamic_buffer<block_size, storage_type,
	tls_block_allocator<typename dynamic_buffer<block_size>::storage_type, count, no_ref_counting, entrant_policy>
>;

} // hexi

// #include <hexi/exception.h>

// #include <hexi/endian.h>

// #include <hexi/file_buffer.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/exception.h>

// #include <hexi/shared.h>

// #include <hexi/concepts.h>

#include <filesystem>
#include <utility>
#include <cstddef>
#include <cstdio>

namespace hexi {

using namespace detail;

class file_buffer final {
public:
	using size_type       = std::size_t;
	using offset_type     = long;
	using value_type      = char;
	using contiguous      = is_non_contiguous;
	using seeking         = unsupported;

	static constexpr auto npos { static_cast<size_type>(-1) };

private:
	FILE* file_ = nullptr;
	offset_type read_ = 0;
	offset_type write_ = 0;
	bool error_ = false;

	/**
	* @brief Explicitly close the underlying file handle.
	* 
	* This function will be called by the object's destructor
	* and does not need to be called explicitly.
	* 
	* @note Idempotent.
	*/
	void close() {
		if(file_) {
			std::fclose(file_);
			file_ = nullptr;
		}
	}

public:
	file_buffer(const std::filesystem::path& path)
		: file_buffer(path.string().c_str()) { }

	file_buffer(const char* path) {
		file_ = std::fopen(path, "a+b");
		
		if(!file_) {
			error_ = true;
			return;
		}
		
		if(std::fseek(file_, 0, SEEK_END)) {
			error_ = true;
			return;
		}

		write_ = std::ftell(file_);

		if(write_ == -1) {
			error_ = true;
		}
	}

	file_buffer(file_buffer&& rhs) noexcept
		: file_(rhs.file_),
		  read_(rhs.read_),
		  write_(rhs.write_),
		  error_(rhs.error_) {
		rhs.file_ = nullptr;
	}

	file_buffer& operator=(file_buffer&& rhs) noexcept {
		std::exchange(file_, rhs.file_);
		read_ = rhs.read_;
		write_ = rhs.write_;
		error_ = rhs.error_;
		return *this;
	}

	file_buffer& operator=(const file_buffer&) = delete;
	file_buffer(const file_buffer&) = delete;

	~file_buffer() {
		close();
	}


	/**
	 * @brief Flush unwritten data.
	 */
	void flush() {
		std::fflush(file_);
	}

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 */
	template<typename T>
	void read(T* destination) {
		read(destination, sizeof(T));
	}

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to read into the buffer.
	 */
	void read(void* destination, size_type length) {
		if(error_) {
			return;
		}

		if(std::fseek(file_, read_, 0)) {
			error_ = true;
			return;
		}

		if(std::fread(destination, length, 1, file_) != 1) {
			error_ = true;
			return;
		}

		read_ += length;
	}

	/**
	 * @brief Copies a number of bytes to the provided buffer but without advancing
	 * the read cursor.
	 * 
	 * @param destination The buffer to copy the data to.
	 */
	template<typename T>
	void copy(T* destination) {
		copy(destination, sizeof(T));
	}

	/**
	 * @brief Copies a number of bytes to the provided buffer but without advancing
	 * the read cursor.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to copy.
	 */
	void copy(void* destination, size_type length) {
		if(error_) {
			return;
		} else if(length > size()) {
			error_ = true;
			HEXI_THROW(buffer_underrun(length, read_, size()));
		}

		if(std::fseek(file_, read_, SEEK_SET)) {
			error_ = true;
			return;
		}
		
		if(std::fread(destination, length, 1, file_) != 1) {
			error_ = true;
			return;
		}
	}

	/**
	 * @brief Attempts to locate the provided value within the container.
	 * 
	 * @param value The value to locate.
	 * 
	 * @return The position of value or npos if not found.
	 */
	size_type find_first_of(value_type val) noexcept {
		if(error_) {
			return npos;
		}

		if(std::fseek(file_, read_, SEEK_SET)) {
			error_ = true;
			return npos;
		}

		value_type buffer{};

		for(size_type i = 0, j = size(); i < j; ++i) {
			if(std::fread(&buffer, sizeof(value_type), 1, file_) != 1) {
				error_ = true;
				return npos;
			}

			if(buffer == val) {
				return i;
			}
		}

		return npos;
	}

	/**
	 * @brief Skip a requested number of bytes.
	 *
	 * Skips over a number of bytes from the file. This should be used
	 * if the file holds data that you don't care about but don't want
	 * to have to read it to another buffer to move beyond it.
	 * 
	 * @param length The number of bytes to skip.
	 */
	void skip(const size_type length) {
		read_ += length;
	}

	/**
	 * @brief Whether the container is empty.
	 * 
	 * @return Returns true if the container is empty (has no data to be read).
	 */
	[[nodiscard]]
	bool empty() const {
		return write_ == read_;
	}

	/**
	 * @brief Determine whether the adaptor supports write seeking.
	 * 
	 * This is determined at compile-time and does not need to checked at
	 * run-time.
	 * 
	 * @return True if write seeking is supported, otherwise false.
	 */
	constexpr static bool can_write_seek() {
		return std::is_same_v<seeking, supported>;
	}

	/**
	 * @brief Write data to the container.
	 * 
	 * @param source Pointer to the data to be written.
	 */
	void write(const auto& source) {
		write(&source, sizeof(source));
	}

	/**
	 * @brief Write provided data to the file.
	 * 
	 * @param source Pointer to the data to be written.
	 * @param length Number of bytes to write from the source.
	 */
	void write(const void* source, size_type length) {
		if(error_) {
			return;
		}
		
		if(std::fseek(file_, write_, SEEK_SET)) {
			error_ = true;
			return;
		}

		if(std::fwrite(source, length, 1, file_) != 1) {
			error_ = true;
			return;
		}

		write_ += length;
	}


	/**
	 * @brief Returns the amount of data available in the file.
	 * 
	 * @return The number of bytes of data available to read from the file.
	 */
	size_type size() const {
		return static_cast<size_type>(write_) - read_;
	}

	/** 
	 * @brief Retrieve the file handle.
	 * 
	 * @return The file handle.
	 */
	FILE* handle() {
		return file_;
	}

	/** 
	 * @brief Retrieve the file handle.
	 * 
	 * @return The file handle.
	 */
	const FILE* handle() const {
		return file_;
	}

	/** 
	 * @return True if an error has occurred during file operations.
	 */
	bool error() const {
		return error_;
	}

	/** 
	 * @return True if no error has occurred during file operations.
	 */
	operator bool() const {
		return !error();
	}
};

} // hexi

// #include <hexi/shared.h>

// #include <hexi/static_buffer.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/exception.h>

// #include <hexi/shared.h>

// #include <hexi/concepts.h>

#include <array>
#include <span>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace hexi {

using namespace detail;

template<byte_type storage_type, std::size_t buf_size>
class static_buffer final {
	std::array<storage_type, buf_size> buffer_ = {};
	std::size_t read_ = 0;
	std::size_t write_ = 0;

public:
	using size_type       = typename decltype(buffer_)::size_type;
	using offset_type     = size_type;
	using value_type      = storage_type;
	using contiguous      = is_contiguous;
	using seeking         = supported;

	static constexpr auto npos { static_cast<size_type>(-1) };
	
	static_buffer() = default;

	template<typename... T> 
	static_buffer(T&&... vals) : buffer_{ std::forward<T>(vals)... } {
		write_ = sizeof... (vals);
	}

	static_buffer(static_buffer&& rhs) = default;
	static_buffer& operator=(static_buffer&&) = default;
	static_buffer& operator=(const static_buffer&) = default;
	static_buffer(const static_buffer&) = default;

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 */
	template<typename T>
	void read(T* destination) {
		read(destination, sizeof(T));
	}

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to read into the buffer.
	 */
	void read(void* destination, size_type length) {
		copy(destination, length);
		read_ += length;

		if(read_ == write_) {
			read_ = write_ = 0;
		}
	}

	/**
	 * @brief Copies a number of bytes to the provided buffer but without advancing
	 * the read cursor.
	 * 
	 * @param destination The buffer to copy the data to.
	 */
	template<typename T>
	void copy(T* destination) const {
		copy(destination, sizeof(T));
	}

	/**
	 * @brief Copies a number of bytes to the provided buffer but without advancing
	 * the read cursor.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to copy.
	 */
	void copy(void* destination, size_type length) const {
		assert(!region_overlap(buffer_.data(), buffer_.size(), destination, length));

		if(length > size()) {
			HEXI_THROW(buffer_underrun(length, read_, size()));
		}

		std::memcpy(destination, read_ptr(), length);
	}

	/**
	 * @brief Attempts to locate the provided value within the container.
	 * 
	 * @param value The value to locate.
	 * 
	 * @return The position of value or npos if not found.
	 */
	size_type find_first_of(value_type val) const noexcept {
		const auto data = read_ptr();

		for(size_type i = 0, j = size(); i < j; ++i) {
			if(data[i] == val) {
				return i;
			}
		}

		return npos;
	}

	/**
	 * @brief Skip over length bytes.
	 * 
	 * Skips over a number of bytes from the container. This should be used
	 * if the container holds data that you don't care about but don't want
	 * to have to read it to another buffer to move beyond it.
	 * 
	 * @param length The number of bytes to skip.
	 */
	void skip(const size_type length) {
		read_ += length;

		if(read_ == write_) {
			read_ = write_ = 0;
		}
	}

	/**
	 * @brief Advances the write cursor.
	 * 
	 * @param size The number of bytes by which to advance the write cursor.
	 */
	void advance_write(size_type bytes) {
		assert(free() >= bytes);
		write_ += bytes;
	}

	/**
	 * @brief Clears the container.
	 */
	void clear() {
		read_ = write_ = 0;
	}

	/**
	 * @brief Moves any unread data to the front of the buffer, freeing space at the end.
	 * If a move is performed, pointers obtained from read/write_ptr() will be invalidated.
	 * 
	 * If the buffer contains no data available for reading, this function will have no effect.
	 * 
	 * @return True if additional space was made available.
	 */
	bool defragment() {
		if(read_ == 0) {
			return false;
		}

		write_ = size();
		std::memmove(buffer_.data(), read_ptr(), write_);
		read_ = 0;
		return true;
	}

	/**
	 * @brief Retrieves a reference to the specified index within the container.
	 * 
	 * @param index The index within the container.
	 * 
	 * @return A reference to the value at the specified index.
	 */
	value_type& operator[](const size_type index) {
		return read_ptr()[index];
	}

	/**
	 * @brief Retrieves a reference to the specified index within the container.
	 * 
	 * @param index The index within the container.
	 * 
	 * @return A reference to the value at the specified index.
	 */
	const value_type& operator[](const size_type index) const {
		return read_ptr()[index];
	}

	/**
	 * @brief Whether the container is empty.
	 * 
	 * @return Returns true if the container is empty (has no data to be read).
	 */
	[[nodiscard]]
	bool empty() const {
		return write_ == read_;
	}

	/**
	 * @return Whether the container is full and cannot be further written to.
	 */
	bool full() const {
		return write_ == capacity();
	}

	/**
	 * @brief Determine whether the adaptor supports write seeking.
	 * 
	 * This is determined at compile-time and does not need to checked at
	 * run-time.
	 * 
	 * @return True if write seeking is supported, otherwise false.
	 */
	constexpr static bool can_write_seek() {
		return std::is_same_v<seeking, supported>;
	}

	/**
	 * @brief Write data to the container.
	 * 
	 * @param source Pointer to the data to be written.
	 */
	void write(const auto& source) {
		write(&source, sizeof(source));
	}

	/**
	 * @brief Write provided data to the container.
	 * 
	 * @param source Pointer to the data to be written.
	 * @param length Number of bytes to write from the source.
	 */
	void write(const void* source, size_type length) {
		assert(!region_overlap(source, length, buffer_.data(), buffer_.size()));

		if(free() < length) {
			HEXI_THROW(buffer_overflow(length, write_, free()));
		}

		std::memcpy(write_ptr(), source, length);
		write_ += length;
	}

	/**
	 * @brief Performs write seeking within the container.
	 * 
	 * @param direction Specify whether to seek in a given direction or to absolute seek.
	 * @param offset The offset relative to the seek direction or the absolute value
	 * when using absolute seeking.
	 */
	void write_seek(const buffer_seek direction, const size_type offset) {
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

	/**
	 * @return An iterator to the beginning of data available for reading.
	 */
	auto begin() {
		return buffer_.begin() + read_;
	}

	/**
	 * @return An iterator to the beginning of data available for reading.
	 */
	auto begin() const {
		return buffer_.begin() + read_;
	}

	/**
	 * @return An iterator to the end of data available for reading.
	 */
	auto end() {
		return buffer_.begin() + write_;
	}

	/**
	 * @return An iterator to the end of data available for reading.
	 */
	auto end() const {
		return buffer_.begin() + write_;
	}

	/**
	 * @brief Overall capacity of the container.
	 * 
	 * @return The container's total size in bytes.
	 */
	constexpr static size_type capacity() {
		return buf_size;
	}

	/**
	 * @brief Returns the size of the container.
	 * 
	 * @return The number of bytes of data available to read within the container.
	 */
	size_type size() const {
		return write_ - read_;
	}

	/**
	 * @brief The amount of free space.
	 * 
	 * @return The number of bytes of free space within the container.
	 */
	size_type free() const {
		return buf_size - write_;
	}

	/**
	 * @return Pointer to the data available for reading.
	 */
	const value_type* data() const {
		return read_ptr();
	}

	/**
	 * @return Pointer to the data available for reading.
	 */
	value_type* data() {
		return read_ptr();
	}

	/**
	 * @return Pointer to the data available for reading.
	 */
	const value_type* read_ptr() const {
		return buffer_.data() + read_;
	}

	/**
	 * @return Pointer to the data available for reading.
	 */
	value_type* read_ptr() {
		return buffer_.data() + read_;
	}

	/**
	 * @return Pointer to the location within the buffer where the next write
	 * will be made.
	 */
	const value_type* write_ptr() const {
		return buffer_.data() + write_;
	}

	/**
	 * @return Pointer to the location within the buffer where the next write
	 * will be made.
	 */
	value_type* write_ptr() {
		return buffer_.data() + write_;
	}

	/**
	 * @return Pointer to the underlying storage.
	 */
	value_type* storage() {
		return buffer_.data();
	}

	/**
	 * @return Pointer to the underlying storage.
	 */
	const value_type* storage() const {
		return buffer_.data();
	}

	/**
	 * @brief Retrieves a span representing the data available for reading contained within
	 * underlying storage.
	 * 
	 * @return A span over the data waiting to be read from the container.
	 */
	std::span<const value_type> read_span() const {
		return { read_ptr(), size() };
	}

	/**
	 * @brief Retrieves a span representing the free space within the underlying storage.
	 * 
	 * @return A span over the container's free space.
	 */
	std::span<value_type> write_span() {
		return { write_ptr(), free() };
	}
};

} // hexi

// #include <hexi/null_buffer.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/pmc/buffer_write.h>

// #include <hexi/shared.h>

// #include <hexi/exception.h>

#include <cstddef>

namespace hexi {

class null_buffer final : public pmc::buffer_write {
public:
	using size_type       = std::size_t;
	using offset_type     = std::size_t;
	using value_type      = std::byte;
	using contiguous      = is_contiguous;
	using seeking         = unsupported;

	void write(const auto& /*elem*/) {}
	void write(const void* /*source*/, size_type /*length*/) override {};
	void read(auto* /*elem*/) {}
	void read(void* /*destination*/, size_type /*length*/) {};
	void copy(auto* /*elem*/) const {}
	void copy(void* /*destination*/, size_type /*length*/) const {};
	void reserve(const size_type /*length*/) override {};
	size_type size() const override{ return 0; };
	[[nodiscard]] bool empty() const override { return true; };
	bool can_write_seek() const override { return false; }

	void write_seek(const buffer_seek /*direction*/, const std::size_t /*offset*/) override {
		HEXI_THROW(exception("Don't do this on a null_buffer")); 
	};
};

} // hexi

// #include <hexi/stream_adaptors.h>

// #include <hexi/allocators/block_allocator.h>

// #include <hexi/allocators/default_allocator.h>

// #include <hexi/allocators/tls_block_allocator.h>

// #include <hexi/detail/intrusive_storage.h>

// #include <hexi/pmc/binary_stream.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/pmc/binary_stream_reader.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/pmc/stream_base.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/pmc/buffer_base.h>

// #include <hexi/shared.h>


namespace hexi::pmc {

class stream_base {
	buffer_base& buffer_;
	stream_state state_;

protected:
	void set_state(stream_state state) {
		state_ = state;
	}

public:
	explicit stream_base(buffer_base& buffer)
		: buffer_(buffer),
		  state_(stream_state::ok) { }

	std::size_t size() const {
		return buffer_.size();
	}

	[[nodiscard]]
	bool empty() const {
		return buffer_.empty();
	}

	stream_state state() const {
		return state_;
	}

	bool good() const {
		return state() == stream_state::ok;
	}

	void clear_state() {
		set_state(stream_state::ok);
	}

	operator bool() const {
		return good();
	}

	void set_error_state() {
		set_state(stream_state::user_defined_err);
	}

	virtual ~stream_base() = default;
};

} // pmc, hexi

// #include <hexi/pmc/buffer_read.h>

// #include <hexi/concepts.h>

// #include <hexi/endian.h>

// #include <hexi/exception.h>

// #include <hexi/shared.h>

// #include <hexi/stream_adaptors.h>

#include <ranges>
#include <string>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace hexi::pmc {

using namespace detail;

class binary_stream_reader : virtual public stream_base {
	buffer_read& buffer_;
	std::size_t total_read_;
	const std::size_t read_limit_;

	void enforce_read_bounds(std::size_t read_size) {
		if(read_size > buffer_.size()) [[unlikely]] {
			set_state(stream_state::buff_limit_err);
			HEXI_THROW(buffer_underrun(read_size, total_read_, buffer_.size()));
		}

		if(read_limit_) {
			const auto max_read_remaining = read_limit_ - total_read_;

			if(read_size > max_read_remaining) [[unlikely]] {
				set_state(stream_state::read_limit_err);
				HEXI_THROW(stream_read_limit(read_size, total_read_, read_limit_));
			}
		}

		total_read_ += read_size;
	}

	inline void read(void* dest, const std::size_t size) {
		if(state() == stream_state::ok) [[likely]] {
			enforce_read_bounds(size);
			buffer_.read(dest, size);
		}
	}

public:
	explicit binary_stream_reader(buffer_read& source, std::size_t read_limit = 0)
		: stream_base(source),
		  buffer_(source),
		  total_read_(0),
		  read_limit_(read_limit) {}

	binary_stream_reader(binary_stream_reader&& rhs) noexcept
		: stream_base(rhs),
		  buffer_(rhs.buffer_), 
		  total_read_(rhs.total_read_),
		  read_limit_(rhs.read_limit_) {
		rhs.total_read_ = static_cast<std::size_t>(-1);
		rhs.set_state(stream_state::invalid_stream);
	}

	binary_stream_reader& operator=(binary_stream_reader&&) = delete;
	binary_stream_reader& operator=(const binary_stream_reader&) = delete;
	binary_stream_reader(const binary_stream_reader&) = delete;

	void deserialise(auto& object) {
		stream_read_adaptor adaptor(*this);
		object.serialise(adaptor);
	}

	binary_stream_reader& operator>>(has_serialise<binary_stream_reader> auto& data) {
		deserialise(data);
		return *this;
	}

	binary_stream_reader& operator>>(prefixed<std::string> adaptor) {
		std::uint32_t size = 0;
		*this >> endian::le(size);

		if(state() != stream_state::ok) {
			return *this;
		}

		enforce_read_bounds(size);

		adaptor->resize_and_overwrite(size, [&](char* strbuf, std::size_t size) {
			buffer_.read(strbuf, size);
			return size;
		});

		return *this;
	}
	
	binary_stream_reader& operator>>(prefixed_varint<std::string> adaptor) {
		const auto size = varint_decode<std::size_t>(*this);

		// if an error was triggered during decode, we shouldn't reach here
		if(state() != stream_state::ok) {
			std::unreachable();
		}

		enforce_read_bounds(size);

		adaptor->resize_and_overwrite(size, [&](char* strbuf, std::size_t size) {
			buffer_.read(strbuf, size);
			return size;
		});

		return *this;
	}

	binary_stream_reader& operator>>(null_terminated<std::string> adaptor) {
		auto pos = buffer_.find_first_of(std::byte{0});

		if(pos == buffer_.npos) {
			adaptor->clear();
			return *this;
		}

		enforce_read_bounds(pos + 1); // include null terminator

		adaptor->resize_and_overwrite(pos, [&](char* strbuf, std::size_t size) {
			buffer_.read(strbuf, pos);
			return size;
		});

		buffer_.skip(1); // skip null terminator
		return *this;
	}

	binary_stream_reader& operator >>(std::string& data) {
		return (*this >> prefixed(data));
	}

	binary_stream_reader& operator>>(has_shr_override<binary_stream_reader> auto&& data) {
		return data.operator>>(*this);
	}

	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	binary_stream_reader& operator>>(endian_func adaptor) {
		read(&adaptor.value, sizeof(adaptor.value));
		adaptor.value = adaptor.from();
		return *this;
	}

	template<pod T>
	requires (!has_shr_override<T, binary_stream_reader>)
	binary_stream_reader& operator>>(T& data) {
		read(&data, sizeof(data));
		return *this;
	}

	binary_stream_reader& operator>>(pod auto& data) {
		read(&data, sizeof(data));
		return *this;
	}

	/**
	 * @brief Reads a string from the stream.
	 * 
	 * @param dest The destination string.
	 */
	void get(std::string& dest) {
		*this >> dest;
	}

	/**
	 * @brief Reads a fixed-length string from the stream.
	 * 
	 * @param dest The destination string.
	 * @param count The number of bytes to be read.
	 */
	void get(std::string& dest, std::size_t size) {
		enforce_read_bounds(size);

		dest.resize_and_overwrite(size, [&](char* strbuf, std::size_t len) {
			buffer_.read(strbuf, len);
			return len;
		});
	}

	/**
	 * @brief Read data from the stream into the provided destination argument.
	 * 
	 * @param dest The destination buffer.
	 * @param count The number of bytes to be read into the destination.
	 */
	template<typename T>
	void get(T* dest, std::size_t count) {
		assert(dest);
		const auto read_size = count * sizeof(T);
		read(dest, read_size);
	}

	/**
	 * @brief Read data from the stream to the destination represented by the iterators.
	 * 
	 * @param begin The beginning iterator.
	 * @param end The end iterator.
	 */
	template<typename It>
	void get(It begin, const It end) {
		for(; begin != end; ++begin) {
			*this >> *begin;
		}
	}

	/**
	 * @brief Read data from the stream into the provided destination argument.
	 * 
	 * @param dest A contiguous range into which the data should be read.
	 */
	template<std::ranges::contiguous_range range>
	void get(range& dest) {
		const auto read_size = dest.size() * sizeof(typename range::value_type);
		read(dest.data(), read_size);
	}

	/**
	 * @brief Read an arithmetic type from the stream.
	 * 
	 * @return The arithmetic value.
	 */
	template<arithmetic T>
	T get() {
		T t{};
		read(&t, sizeof(T));
		return t;
	}

	/**
	 * @brief Read an arithmetic type from the stream.
	 * 
	 * @return The arithmetic value.
	 */
	void get(arithmetic auto& dest) {
		read(&dest, sizeof(dest));
	}

	/**
	 * @brief Read an arithmetic type from the stream, allowing for endian
	 * conversion.
	 * 
	 * @param The destination for the read value.
	 */
	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	void get(endian_func& adaptor) {
		read(&adaptor.value, sizeof(adaptor.value));
		adaptor.value = adaptor.from();
	}

	/**
	 * @brief Read an arithmetic type from the stream, allowing for endian
	 * conversion.
	 * 
	 * @return The arithmetic value.
	 */
	template<arithmetic T, endian::conversion conversion>
	T get() {
		T t{};
		read(&t, sizeof(T));
		return endian::convert<conversion>(t);
	}

	/**  Misc functions **/ 

	/**
	 * @brief Skip over count bytes
	 *
	 * Skips over a number of bytes from the container. This should be used
	 * if the container holds data that you don't care about but don't want
	 * to have to read it to another buffer to move beyond it.
	 * 
	 * @param length The number of bytes to skip.
	 */
	void skip(std::size_t count) {
		enforce_read_bounds(count);
		buffer_.skip(count);
	}

	/**
	 * @return The total number of bytes read from the stream.
	 */
	std::size_t total_read() const {
		return total_read_;
	}

	/**
	 * @return If provided to the constructor, the upper limit on how much data
	 * can be read from this stream before an error is triggers.
	 */
	std::size_t read_limit() const {
		return read_limit_;
	}

	/**
	 * @brief Determine the maximum number of bytes that can be
	 * safely read from this stream.
	 * 
	 * The value returned may be lower than the amount of data
	 * available in the buffer if a read limit was set during
	 * the stream's construction.
	 * 
	 * @return The number of bytes available for reading.
	 */
	std::size_t read_max() const {
		if(read_limit_) {
			return read_limit_ - total_read_;
		} else {
			return buffer_.size();
		}
	}

	/**
	 * @return Pointer to stream's underlying buffer.
	 */
	buffer_read* buffer() const {
		return &buffer_;
	}
};

} // pmc, hexi

// #include <hexi/pmc/binary_stream_writer.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/pmc/stream_base.h>

// #include <hexi/pmc/buffer_write.h>

// #include <hexi/concepts.h>

// #include <hexi/endian.h>

// #include <hexi/shared.h>

// #include <hexi/stream_adaptors.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <type_traits>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace hexi::pmc {

using namespace detail;

class binary_stream_writer : virtual public stream_base {
	buffer_write& buffer_;
	std::size_t total_write_;

	inline void write(const void* data, const std::size_t size) {
		if(state() == stream_state::ok) [[likely]] {
			buffer_.write(data, size);
			total_write_ += size;
		}
	}

public:
	explicit binary_stream_writer(buffer_write& source)
		: stream_base(source),
		  buffer_(source),
		  total_write_(0) {}

	binary_stream_writer(binary_stream_writer&& rhs) noexcept
		: stream_base(rhs),
		  buffer_(rhs.buffer_), 
		  total_write_(rhs.total_write_) {
		rhs.total_write_ = static_cast<std::size_t>(-1);
		rhs.set_state(stream_state::invalid_stream);
	}

	binary_stream_writer& operator=(binary_stream_writer&&) = delete;
	binary_stream_writer& operator=(const binary_stream_writer&) = delete;
	binary_stream_writer(const binary_stream_writer&) = delete;

	void serialise(auto&& object) {
		stream_write_adaptor adaptor(*this);
		object.serialise(adaptor);
	}

	binary_stream_writer& operator<<(has_shl_override<binary_stream_writer> auto&& data) {
		return data.operator<<(*this);
	}

	template<typename T>
	requires has_serialise<T, stream_write_adaptor<binary_stream_writer>>
	binary_stream_writer& operator<<(T& data) {
		serialise(data);
		return *this;
	}

	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	binary_stream_writer& operator<<(endian_func adaptor) {
		const auto converted = adaptor.to();
		write(&converted, sizeof(converted));
		return *this;
	}

	template<pod T>
	requires (!has_shl_override<T, binary_stream_writer>)
	binary_stream_writer& operator<<(const T& data) {
		write(&data, sizeof(data));
		return *this;
	}

	template<typename T>
	binary_stream_writer& operator<<(prefixed<T> adaptor) {
		auto size = static_cast<std::uint32_t>(adaptor->size());
		endian::native_to_little_inplace(size);
		write(&size, sizeof(size));
		write(adaptor->data(), adaptor->size());
		return *this;
	}

	template<typename T>
	binary_stream_writer& operator<<(prefixed_varint<T> adaptor) {
		varint_encode(*this, adaptor->size());
		write(adaptor->data(), adaptor->size());
		return *this;
	}

	template<typename T>
	requires std::is_same_v<std::decay_t<T>, std::string_view>
	binary_stream_writer& operator<<(null_terminated<T> adaptor) {
		assert(adaptor->find_first_of('\0') == adaptor->npos);
		write(adaptor->data(), adaptor->size());
		const char terminator = '\0';
		write(&terminator, 1);
		return *this;
	}

	template<typename T>
	requires std::is_same_v<std::decay_t<T>, std::string>
	binary_stream_writer& operator<<(null_terminated<T> adaptor) {
		assert(adaptor->find_first_of('\0') == adaptor->npos);
		write(adaptor->data(), adaptor->size() + 1); // yes, the standard allows this
		return *this;
	}

	template<typename T>
	binary_stream_writer& operator<<(raw<T> adaptor) {
		write(adaptor->data(), adaptor->size());
		return *this;
	}

	binary_stream_writer& operator<<(std::string_view string) {
		return (*this << prefixed(string));
	}

	binary_stream_writer& operator<<(const std::string& string) {
		return (*this << prefixed(string));
	}

	binary_stream_writer& operator<<(const char* data) {
		assert(data);
		const auto len = std::strlen(data);
		write(data, len + 1); // include terminator
		return *this;
	}

	template<std::ranges::contiguous_range range>
	requires pod<typename range::value_type>
	binary_stream_writer& operator <<(const range& data) {
		const auto write_size = data.size() * sizeof(typename range::value_type);
		write(data.data(), write_size);
		return *this;
	}

	template<is_iterable T>
	requires (!pod<typename T::value_type> || !std::ranges::contiguous_range<T>)
	binary_stream_writer& operator<<(T& data) {
		for(auto& element : data) {
			*this << element;
		}

		return *this;
	}

	/**
	 * @brief Writes a contiguous range to the stream.
	 * 
	 * @param data The contiguous range to be written to the stream.
	 */
	template<std::ranges::contiguous_range range>
	void put(range& data) {
		const auto write_size = data.size() * sizeof(typename range::value_type);
		write(data.data(), write_size);
	}

	/**
	 * @brief Writes a the provided value to the stream.
	 * 
	 * @param data The value to be written to the stream.
	 */
	void put(const arithmetic auto& data) {
		write(&data, sizeof(data));
	}

	/**
	 * @brief Writes data to the stream.
	 * 
	 * @param data The element to be written to the stream.
	 */
	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	void put(const endian_func& adaptor) {
		const auto swapped = adaptor.to();
		write(&swapped, sizeof(swapped));
	}

	/**
	 * @brief Writes count elements from the provided buffer to the stream.
	 * 
	 * @param data Pointer to the buffer from which data will be copied to the stream.
	 * @param count The number of elements to write.
	 */
	template<pod T>
	void put(const T* data, std::size_t count) {
		assert(data);
		const auto write_size = count * sizeof(T);
		write(data, write_size);
	}

	/**
	 * @brief Writes the data from the iterator range to the stream.
	 * 
	 * @param begin Iterator to the beginning of the data.
	 * @param end Iterator to the end of the data.
	 */
	template<typename It>
	void put(It begin, const It end) {
		for(auto it = begin; it != end; ++it) {
			*this << *it;
		}
	}
	/**
	 * @brief Allows for writing a provided byte value a specified number of times to
	 * the stream.
	 * 
	 * @param The byte value that will fill the specified number of bytes.
	 */
	template<std::size_t size>
	void fill(const std::uint8_t value) {
		const auto filled = generate_filled<size>(value);
		write(filled.data(), filled.size());
	}

	/**  Misc functions **/ 

	/**
	 * @brief Determines whether this container can write seek.
	 * 
	 * @return Whether this container is capable of write seeking.
	 */
	bool can_write_seek() const {
		return buffer_.can_write_seek();
	}

	/**
	 * @brief Performs write seeking within the container.
	 * 
	 * @param direction Specify whether to seek in a given direction or to absolute seek.
	 * @param offset The offset relative to the seek direction or the absolute value
	 * when using absolute seeking.
	 */
	void write_seek(const stream_seek direction, const std::size_t offset) {
		if(direction == stream_seek::sk_stream_absolute) {
			if(offset >= total_write_) {
				buffer_.write_seek(buffer_seek::sk_forward, offset - total_write_);
			} else {
				buffer_.write_seek(buffer_seek::sk_backward, total_write_ - offset);
			}

			total_write_ = offset;
		} else {
			buffer_.write_seek(static_cast<buffer_seek>(direction), offset);
		}
	}

	/**
	 * @brief Returns the size of the container.
	 * 
	 * @return The number of bytes of data available to read within the stream.
	 */
	std::size_t size() const {
		return buffer_.size();
	}

	/**
	 * @brief Whether the container is empty.
	 * 
	 * @return Returns true if the container is empty (has no data to be read).
	 */
	[[nodiscard]]
	bool empty() const {
		return buffer_.empty();
	}

	/**
	 * @return The total number of bytes written to the stream.
	 */
	std::size_t total_write() const {
		return total_write_;
	}

	/** 
	 * @brief Get a pointer to the buffer.
	 *
	 * @return Pointer to the underlying buffer. 
	 */
	buffer_write* buffer() {
		return &buffer_;
	}

	/** 
	 * @brief Get a pointer to the buffer.
	 *
	 * @return Pointer to the underlying buffer. 
	 */
	const buffer_write* buffer() const {
		return &buffer_;
	}
};

} // pmc, hexi

// #include <hexi/pmc/stream_base.h>

// #include <hexi/pmc/buffer.h>

#include <cstddef>

namespace hexi::pmc {

class binary_stream final : public binary_stream_reader, public binary_stream_writer {
public:
	explicit binary_stream(hexi::pmc::buffer& source, std::size_t read_limit = 0)
		: stream_base(source),
		  binary_stream_reader(source, read_limit),
		  binary_stream_writer(source) {}

	~binary_stream() override = default;
};

} // pmc, hexi

// #include <hexi/pmc/binary_stream_reader.h>

// #include <hexi/pmc/binary_stream_writer.h>

// #include <hexi/pmc/buffer.h>

// #include <hexi/pmc/buffer_adaptor.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/pmc/buffer.h>

// #include <hexi/pmc/buffer_read_adaptor.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/pmc/buffer_read.h>

// #include <hexi/shared.h>

// #include <hexi/concepts.h>

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

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 */
	template<typename T>
	void read(T* destination) {
		read(destination, sizeof(T));
	}

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to read into the buffer.
	 */
	void read(void* destination, std::size_t length) override {
		assert(destination && !region_overlap(buffer_.data(), buffer_.size(), destination, length));
		std::memcpy(destination, buffer_.data() + read_, length);
		read_ += length;
	}

	/**
	 * @brief Copies a number of bytes to the provided buffer but without advancing
	 * the read cursor.
	 * 
	 * @param destination The buffer to copy the data to.
	 */
	template<typename T>
	void copy(T* destination) const {
		copy(destination, sizeof(T));
	}

	/**
	 * @brief Copies a number of bytes to the provided buffer but without advancing
	 * the read cursor.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to copy.
	 */
	void copy(void* destination, std::size_t length) const override {
		assert(destination && !region_overlap(buffer_.data(), buffer_.size(), destination, length));
		std::memcpy(destination, buffer_.data() + read_, length);
	}

	/**
	 * @brief Skip over requested number of bytes.
	 *
	 * Skips over a number of bytes from the container. This should be used
	 * if the container holds data that you don't care about but don't want
	 * to have to read it to another buffer to move beyond it.
	 * 
	 * @param length The number of bytes to skip.
	 */
	void skip(std::size_t length) override {
		read_ += length;
	}

	/**
	 * @brief Returns the size of the container.
	 * 
	 * @return The number of bytes of data available to read within the stream.
	 */
	std::size_t size() const override {
		return buffer_.size() - read_;
	}

	/**
	 * @brief Whether the container is empty.
	 * 
	 * @return Returns true if the container is empty (has no data to be read).
	 */
	[[nodiscard]]
	bool empty() const override {
		return !(buffer_.size() - read_);
	}

	/**
	 * @brief Retrieves a reference to the specified index within the container.
	 * 
	 * @param index The index within the container.
	 * 
	 * @return A reference to the value at the specified index.
	 */
	const std::byte& operator[](const std::size_t index) const override {
		return reinterpret_cast<const std::byte*>(buffer_.data() + read_)[index];
	}

	/**
	 * @return Pointer to the data available for reading.
	 */
	auto read_ptr() const {
		return buffer_.data() + read_;
	}

	/**
	 * @return The current read offset.
	 */
	auto read_offset() const {
		return read_;
	}

	/**
	 * @brief Attempts to locate the provided value within the container.
	 * 
	 * @param value The value to locate.
	 * 
	 * @return The position of value or npos if not found.
	 */
	std::size_t find_first_of(std::byte val) const override {
		const auto data = read_ptr();

		for(std::size_t i = 0, j = size(); i < j; ++i) {
			if(static_cast<std::byte>(data[i]) == val) {
				return i;
			}
		}

		return npos;
	}

	/**
	 * @brief Clear the underlying buffer and reset state.
	 */
	void clear() {
		read_ = 0;

		if constexpr(has_clear<buf_type>) {
			buffer_.clear();
		}
	}
};

} // pmc, hexi

// #include <hexi/pmc/buffer_write_adaptor.h>
//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi



// #include <hexi/pmc/buffer_write.h>

// #include <hexi/shared.h>

// #include <hexi/concepts.h>

// #include <hexi/exception.h>

#include <ranges>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace hexi::pmc {

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

	buffer_write_adaptor(buf_type& buffer, init_empty_t)
		: buffer_(buffer),
		  write_(0) {}

	/**
	 * @brief Write data to the container.
	 * 
	 * @param source Pointer to the data to be written.
	 */
	void write(auto& source) {
		write(&source, sizeof(source));
	}

	/**
	 * @brief Write provided data to the container.
	 *
	 * @param source Pointer to the data to be written.
	 * @param length Number of bytes to write from the source.
	 */
	void write(const void* source, std::size_t length) override {
		assert(source && !region_overlap(source, length, buffer_.data(), buffer_.size()));
		const auto min_req_size = write_ + length;

		if(buffer_.size() < min_req_size) {
			if constexpr(has_resize_overwrite<buf_type>) {
				buffer_.resize_and_overwrite(min_req_size, [](char*, std::size_t size) {
					return size;
											 });
			} else if constexpr(has_resize<buf_type>) {
				buffer_.resize(min_req_size);
			} else {
				HEXI_THROW(buffer_overflow(free(), length, write_));
			}
		}

		std::memcpy(buffer_.data() + write_, source, length);
		write_ += length;
	}

	/**
	 * @brief Reserves a number of bytes within the container for future use.
	 *
	 * @param length The number of bytes that the container should reserve.
	 */
	void reserve(const std::size_t length) override {
		if constexpr(has_reserve<buf_type>) {
			buffer_.reserve(length);
		}
	}

	/**
	 * @brief Determines whether this container can write seek.
	 *
	 * @return Whether this container is capable of write seeking.
	 */
	bool can_write_seek() const override {
		return true;
	}

	/**
	 * @brief Performs write seeking within the container.
	 *
	 * @param direction Specify whether to seek in a given direction or to absolute seek.
	 * @param offset The offset relative to the seek direction or the absolute value
	 * when using absolute seeking.
	 */
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

	/**
	 * @return Pointer to the underlying storage.
	 */
	auto storage() const {
		return buffer_.data();
	}

	/**
	 * @return Pointer to the underlying storage.
	 */
	auto storage() {
		return buffer_.data();
	}

	/**
	 * @return Pointer to the location within the buffer where the next write
	 * will be made.
	 */
	auto write_ptr() {
		return buffer_.data() + write_;
	}

	/**
	 * @return Pointer to the location within the buffer where the next write
	 * will be made.
	 */
	auto write_ptr() const {
		return buffer_.data() + write_;
	}

	/**
	 * @brief Clear the underlying buffer and reset state.
	 */
	void clear() {
		write_ = 0;

		if constexpr(has_clear<buf_type>) {
			buffer_.clear();
		}
	}

	std::size_t free() const {
		return buffer_.size() - write_;
	}
};

} // pmc, hexi

// #include <hexi/concepts.h>

#include <ranges>

namespace hexi::pmc {

template<byte_oriented buf_type, bool allow_optimise  = true>
requires std::ranges::contiguous_range<buf_type>
class buffer_adaptor final : public buffer_read_adaptor<buf_type>,
                             public buffer_write_adaptor<buf_type>,
                             public buffer {
	void conditional_clear() {
		if(buffer_read_adaptor<buf_type>::read_ptr() == buffer_write_adaptor<buf_type>::write_ptr()) {
			clear();
		}
	}

public:
	explicit buffer_adaptor(buf_type& buffer)
		: buffer_read_adaptor<buf_type>(buffer),
		  buffer_write_adaptor<buf_type>(buffer) {}

	explicit buffer_adaptor(buf_type& buffer, init_empty_t)
		: buffer_read_adaptor<buf_type>(buffer),
		  buffer_write_adaptor<buf_type>(buffer, init_empty) {}

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 */
	template<typename T>
	void read(T* destination) {
		buffer_read_adaptor<buf_type>::read(destination);

		if constexpr(allow_optimise) {
			conditional_clear();
		}
	}

	/**
	 * @brief Reads a number of bytes to the provided buffer.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to read into the buffer.
	 */
	void read(void* destination, std::size_t length) override {
		buffer_read_adaptor<buf_type>::read(destination, length);

		if constexpr(allow_optimise) {
			conditional_clear();
		}
	};

	/**
	 * @brief Write data to the container.
	 * 
	 * @param source Pointer to the data to be written.
	 */
	void write(const auto& source) {
		buffer_write_adaptor<buf_type>::write(source);
	};

	/**
	 * @brief Write provided data to the container.
	 * 
	 * @param source Pointer to the data to be written.
	 * @param length Number of bytes to write from the source.
	 */
	void write(const void* source, std::size_t length) override {
		buffer_write_adaptor<buf_type>::write(source, length);
	};

	void copy(auto* destination) const {
		buffer_read_adaptor<buf_type>::copy(destination);
	};

	/**
	 * @brief Copies a number of bytes to the provided buffer but without advancing
	 * the read cursor.
	 * 
	 * @param destination The buffer to copy the data to.
	 * @param length The number of bytes to copy.
	 */
	void copy(void* destination, std::size_t length) const override {
		buffer_read_adaptor<buf_type>::copy(destination, length);
	};

	/**
	 * @brief Skip over requested number of bytes.
	 *
	 * Skips over a number of bytes from the container. This should be used
	 * if the container holds data that you don't care about but don't want
	 * to have to read it to another buffer to move beyond it.
	 * 
	 * @param length The number of bytes to skip.
	 */
	void skip(std::size_t length) override {
		buffer_read_adaptor<buf_type>::skip(length);

		if constexpr(allow_optimise) {
			conditional_clear();
		}
	};

	/**
	 * @brief Retrieves a reference to the specified index within the container.
	 * 
	 * @param index The index within the container.
	 * 
	 * @return A reference to the value at the specified index.
	 */
	const std::byte& operator[](const std::size_t index) const override { 
		return buffer_read_adaptor<buf_type>::operator[](index); 
	};

	/**
	 * @brief Retrieves a reference to the specified index within the container.
	 * 
	 * @param index The index within the container.
	 * 
	 * @return A reference to the value at the specified index.
	 */
	std::byte& operator[](const std::size_t index) override {
		const auto offset = buffer_read_adaptor<buf_type>::read_offset();
		auto buffer = buffer_write_adaptor<buf_type>::storage();
		return reinterpret_cast<std::byte*>(buffer + offset)[index];
	}

	/**
	 * @brief Reserves a number of bytes within the container for future use.
	 * 
	 * @param length The number of bytes that the container should reserve.
	 */
	void reserve(std::size_t length) override {
		buffer_write_adaptor<buf_type>::reserve(length);
	};

	/**
	 * @brief Determines whether this container can write seek.
	 * 
	 * @return Whether this container is capable of write seeking.
	 */
	bool can_write_seek() const override { 
		return buffer_write_adaptor<buf_type>::can_write_seek();
	};

	/**
	 * @brief Performs write seeking within the container.
	 * 
	 * @param direction Specify whether to seek in a given direction or to absolute seek.
	 * @param offset The offset relative to the seek direction or the absolute value
	 * when using absolute seeking.
	 */
	void write_seek(buffer_seek direction, std::size_t offset) override {
		buffer_write_adaptor<buf_type>::write_seek(direction, offset);
	};

	/**
	 * @brief Returns the size of the container.
	 * 
	 * @return The number of bytes of data available to read within the stream.
	 */
	std::size_t size() const override { 
		return buffer_read_adaptor<buf_type>::size(); 
	};

	/**
	 * @brief Whether the container is empty.
	 * 
	 * @return Returns true if the container is empty (has no data to be read).
	 */
	[[nodiscard]]
	bool empty() const override { 
		return buffer_read_adaptor<buf_type>::empty();
	}

	/**
	 * @brief Attempts to locate the provided value within the container.
	 * 
	 * @param value The value to locate.
	 * 
	 * @return The position of value or npos if not found.
	 */
	std::size_t find_first_of(std::byte val) const override { 
		return buffer_read_adaptor<buf_type>::find_first_of(val);
	}

	void clear() {
		buffer_read_adaptor<buf_type>::clear();
		buffer_write_adaptor<buf_type>::clear();
	}
};

} // pmc, hexi

// #include <hexi/pmc/buffer_base.h>

// #include <hexi/pmc/buffer_read.h>

// #include <hexi/pmc/buffer_read_adaptor.h>

// #include <hexi/pmc/buffer_write.h>

// #include <hexi/pmc/buffer_write_adaptor.h>

// #include <hexi/pmc/stream_base.h>

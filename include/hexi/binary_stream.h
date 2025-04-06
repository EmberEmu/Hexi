//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/shared.h>
#include <hexi/concepts.h>
#include <hexi/exception.h>
#include <hexi/endian.h>
#include <hexi/stream_adaptors.h>
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
		total_write_ += sizeof(T);
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

	template<typename container_type>
	void write_container(container_type& container) {
		using cvalue_type = typename container_type::value_type;

		if constexpr(memcpy_write<container_type, binary_stream>) {
			const auto bytes = container.size() * sizeof(cvalue_type);
			write(container.data(), static_cast<size_type>(bytes));
		} else {
			for(auto& element : container) {
				*this << element;
			}
		}
	}

	template<typename container_type, typename count_type>
	void read_container(container_type& container, const count_type count) {
		using cvalue_type = typename container_type::value_type;

		if constexpr(!memcpy_read<container_type, binary_stream>) {
			container.clear();
		}

		if constexpr(has_reserve<container_type>) {
			container.reserve(count);
		}

		if constexpr(memcpy_read<container_type, binary_stream>) {
			container.resize(count);

			const auto bytes = static_cast<size_type>(count * sizeof(cvalue_type));
			SAFE_READ(container.data(), bytes, void());
		} else {
			for(count_type i = 0; i < count; ++i) {
				cvalue_type value;
				*this >> value;
				container.emplace_back(std::move(value));
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

	/**
	 * @brief Serialises an object that provides a serialise function:
	 * void serialise(auto& stream);
	 * 
	 * @param object The object to be serialised.
	 */
	void serialise(auto&& object) requires writeable<buf_type> {
		stream_write_adaptor adaptor(*this);
		object.serialise(adaptor);
	}

	/**
	 * @brief Serialises an object that provides a serialise function:
	 * void serialise(auto& stream);
	 * 
	 * @param data The object to be serialised.
	 * 
	 * @return Reference to the current stream.
	 */
	template<typename T>
	requires has_serialise<T, binary_stream>
	binary_stream& operator<<(T& data) requires writeable<buf_type> {
		serialise(data);
		return *this;
	}

	/**
	 * @brief Serialises an object that provides a serialise function:
	 * auto& operator<<(auto& stream);
	 * 
	 * @param data The object to be serialised.
	 * 
	 * @return Reference to the current stream.
	 */
	binary_stream& operator<<(const has_shl_override<binary_stream> auto& data)
	requires writeable<buf_type> {
		return *this << data;
	}

	/**
	 * @brief Serialises an arithmetic type with the requested byte order.
	 * 
	 * @param adaptor An endian adaptor.
	 * 
	 * @return Reference to the current stream.
	 */
	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	binary_stream& operator<<(endian_func adaptor) requires writeable<buf_type> {
		const auto converted = adaptor.to();
		write(&converted, sizeof(converted));
		return *this;
	}

	/**
	 * @brief Serialises an arithmetic type with the default byte order.
	 * 
	 * @param data Reference to the type to be serialised.
	 * 
	 * @return Reference to the current stream.
	 */
	binary_stream& operator<<(const arithmetic auto& data) requires writeable<buf_type> {
		const auto converted = endian::storage_in(data, byte_order);
		write(&converted, sizeof(converted));
		return *this;
	}

	/**
	 * @brief Serialises a POD type.
	 * 
	 * @tparam T The type of the POD object.
	 * @param data Reference to the object to be serialised.
	 * 
	 * @note Will not be used for types with user-defined serialisation
	 * functions.
	 * 
	 * @return Reference to the current stream.
	 */
	template<pod T>
	requires (!has_shl_override<T, binary_stream> && !arithmetic<T>)
	binary_stream& operator<<(const T& data) requires writeable<buf_type> {
		write(&data, sizeof(T));
		return *this;
	}

	/**
	 * @brief Serialises a string or string_view with a fixed-length prefix
	 * 
	 * @tparam T The string type.
	 * @param adaptor prefixed adaptor that will instruct the stream to write a length
	 * prefix before serialising the string.
	 * 
	 * @return Reference to the current stream.
	 */
	template<typename T>
	requires std::is_same_v<std::decay_t<T>, std::string_view>
	binary_stream& operator<<(null_terminated<T> adaptor) requires writeable<buf_type> {
		assert(adaptor->find_first_of('\0') == adaptor->npos);
		write(adaptor->data(), adaptor->size());
		write('\0');
		return *this;
	}

	/**
	 * @brief Serialises a string_view as a null terminated string.
	 * 
	 * @tparam T The string type.
	 * @param adaptor null_terminated adaptor that will instruct the stream to write
	 * a null terminated string with no prefix.
	 * 
	 * @return Reference to the current stream.
	 */
	template<typename T>
	requires std::is_same_v<std::decay_t<T>, std::string>
	binary_stream& operator<<(null_terminated<T> adaptor) requires writeable<buf_type> {
		assert(adaptor->find_first_of('\0') == adaptor->npos);
		write(adaptor->data(), adaptor->size() + 1); // yes, the standard allows this
		return *this;
	}

	/**
	 * @brief Serialises a string, string_view or any type providing data()
	 * and a size() member functions.
	 * 
	 * @tparam T The type.
	 * @param adaptor raw adaptor referencing the type to be serialised.
	 * 
	 * @return Reference to the current stream.
	 */
	template<typename T>
	binary_stream& operator<<(raw<T> adaptor) requires writeable<buf_type> {
		write(adaptor->data(), adaptor->size());
		return *this;
	}

	/**
	 * @brief Serialises an std::string_view with a fixed-length prefix.
	 * 
	 * @param string std::string_view to be serialised with a prefix.
	 * 
	 * @return Reference to the current stream.
	 */
	binary_stream& operator<<(std::string_view string) requires writeable<buf_type> {
		return *this << prefixed(string);
	}

	/**
	 * @brief Serialises an std::string with a fixed-length prefix.
	 * 
	 * @param string std::string to be serialised with a prefix.
	 * 
	 * @return Reference to the current stream.
	 */
	binary_stream& operator<<(const std::string& string) requires writeable<buf_type> {
		return *this << prefixed(string);
	}

	/**
	 * @brief Serialises a C-style string.
	 * 
	 * @param data string to be serialised.
	 * 
	 * @note The null terminator for this string will also be written. It will
	 * not be prefixed.
	 * 
	 * @return Reference to the current stream.
	 */
	binary_stream& operator<<(const char* data) requires writeable<buf_type> {
		assert(data);
		const auto len = std::strlen(data);
		write(data, len + 1); // include terminator
		return *this;
	}

	binary_stream& operator<<(const is_iterable auto& data) requires writeable<buf_type> {
		write_container(data);
		return *this;
	}

	/**
	 * @brief Serialises an iterable container.
	 * 
	 * @tparam T The iterable container type.
	 * @param adaptor The container to be serialised.
	 * 
	 * @note A fixed-length prefix representing the number of elements in the
	 * container will be written before the elements.
	 * 
	 * @return Reference to the current stream.
	 */
	template<is_iterable T>
	binary_stream& operator<<(prefixed<T> adaptor) requires writeable<buf_type> {
		const auto count = static_cast<std::uint32_t>(adaptor->size());
		write(endian::native_to_little(count));
		write_container(adaptor.str);
		return *this;
	}

	/**
	 * @brief Serialises an iterable container.
	 * 
	 * @tparam T The iterable container type.
	 * @param adaptor The container to be serialised.
	 * 
	 * @note A variable-length prefix representing the number of elements in the
	 * container will be written before the elements.
	 * 
	 * @return Reference to the current stream.
	 */
	template<is_iterable T>
	binary_stream& operator<<(prefixed_varint<T> adaptor) requires writeable<buf_type> {
		varint_encode(*this, adaptor->size());
		write_container(adaptor.str);
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
	 * @param adaptor The element to be written to the stream.
	 */
	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	void put(const endian_func& adaptor) requires writeable<buf_type> {
		const auto swapped = adaptor.to();
		write(&swapped, sizeof(swapped));
	}

	/**
	 * @brief Writes count elements from the provided buffer to the stream.
	 * 
	 * @tparam T POD type.
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
	 * @tparam It The iterator type.
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
	 * @tparam size The number of bytes to generate.
	 * @param value The byte value that will fill the specified number of bytes.
	 */
	template<size_type size>
	constexpr void fill(const std::uint8_t value) requires writeable<buf_type> {
		const auto filled = generate_filled<size>(value);
		write(filled.data(), filled.size());
	}

	/*** Read ***/

	/**
	 * @brief Deserialises an object that provides a deserialise function:
	 * void serialise(auto& stream);
	 * 
	 * @param[out] object The object to be deserialised.
	 */
	void deserialise(auto& object) {
		stream_read_adaptor adaptor(*this);
		object.serialise(adaptor);
	}

	/**
	 * @brief Deserialises an object that provides a deserialise function:
	 * void serialise(auto& stream);
	 * 
	 * @param[out] data The object to be serialised.
	 * 
	 * @return Reference to the current stream.
	 */
	template<typename T>
	requires has_deserialise<T, binary_stream>
	binary_stream& operator>>(T& data) {
		deserialise(data);
		return *this;
	}

	/**
	 * @brief Deserialises a string that was previously written with a
	 * fixed-length prefix.
	 * 
	 * @param[out] adaptor std::string to hold the result.
	 * 
	 * @return Reference to the current stream.
	 */
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

	/**
	 * @brief Deserialises a string that was previously written with a
	 * fixed-length prefix.
	 * 
	 * @param[out] adaptor std::string_view to hold the result.
	 * 
	 * @note The string_view's lifetime is tied to that of the underlying
	 * buffer. You should not make any further writes to the buffer
	 * unless you know it will not cause the data to be overwritten or
	 * relocated (std::vector).
	 * 
	 * @return Reference to the current stream.
	 */
	binary_stream& operator>>(prefixed<std::string_view> adaptor) {
		std::uint32_t size = 0;
		*this >> endian::le(size);

		if(state_ != stream_state::ok) {
			return *this;
		}

		adaptor.str = std::string_view { span<char>(size) };
		return *this;
	}
	
	/**
	 * @brief Deserialises a string that was previously written with a
	 * variable-length prefix.
	 * 
	 * @param[out] adaptor std::string to hold the result.
	 * 
	 * @return Reference to the current stream.
	 */
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

	/**
	 * @brief Deserialises a string that was previously written with a
	 * variable-length prefix.
	 * 
	 * @param[out] adaptor std::string_view to hold the result.
	 * 
	 * @note The string_view's lifetime is tied to that of the underlying
	 * buffer. You should not make any further writes to the buffer
	 * unless you know it will not cause the data to be overwritten or
	 * relocated (std::vector).
	 * 
	 * @return Reference to the current stream.
	 */
	binary_stream& operator>>(prefixed_varint<std::string_view> adaptor) {
		const auto size = varint_decode<size_type>(*this);

		// if an error was triggered during decode
		if(state_ != stream_state::ok) {
			return *this;
		}
		
		adaptor.str = std::string_view { span<char>(size) };
		return *this;
	}

	/**
	 * @brief Deserialises a string that was previously written with a
	 * null terminator.
	 * 
	 * @param[out] adaptor std::string to hold the result.
	 * 
	 * @return Reference to the current stream.
	 */
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

		// no need to enforce bounds, we know there's enough data
		buffer_.skip(1); // skip null terminator
		return *this;
	}

	/**
	 * @brief Deserialises a string that was previously written with a
	 * null terminator.
	 * 
	 * @param[out] adaptor std::string_view to hold the result.
	 * 
	 * @note The string_view's lifetime is tied to that of the underlying
	 * buffer. You should not make any further writes to the buffer
	 * unless you know it will not cause the data to be overwritten (i.e.
	 * buffer adaptor with optimise disabled).
	 * 
	 * @return Reference to the current stream.
	 */
	binary_stream& operator>>(null_terminated<std::string_view> adaptor) {
		adaptor.str = view();
		return *this;
	}

	/**
	 * @brief Deserialises a string that was previously written with a
	 * fixed-length prefix.
	 * 
	 * @param[out] data std::string_view to hold the result.
	 * 
	 * @note The string_view's lifetime is tied to that of the underlying
	 * buffer. You should not make any further writes to the buffer
	 * unless you know it will not cause the data to be overwritten or
	 * relocated (std::vector).
	 * 
	 * @return Reference to the current stream.
	 */
	binary_stream& operator>>(std::string_view& data) {
		return *this >> prefixed(data);
	}

	/**
	 * @brief Deserialises a string that was previously written with a
	 * fixed-length prefix.
	 * 
	 * @param[out] data std::string to hold the result.
	 * 
	 * @return Reference to the current stream.
	 */
	binary_stream& operator>>(std::string& data) {
		return *this >> prefixed(data);
	}

	/**
	 * @brief Deserialises an object that provides a deserialise function:
	 * auto& operator>>(auto& stream);
	 * 
	 * @param[out] data The object to be deserialised.
	 * 
	 * @return Reference to the current stream.
	 */
	binary_stream& operator>>(has_shr_override<binary_stream> auto&& data) {
		return *this >> data;
	}

	/**
	 * @brief Deserialises an arithmetic type with the requested byte order.
	 * 
	 * @param[out] endian_func The arithmetic type wrapped by the adaptor.
	 * 
	 * @return Reference to the current stream.
	 */
	template<std::derived_from<endian::adaptor_tag_t> endian_func>
	binary_stream& operator>>(endian_func adaptor) {
		SAFE_READ(&adaptor.value, sizeof(adaptor.value), *this);
		adaptor.value = adaptor.from();
		return *this;
	}

	/**
	 * @brief Deserialises an arithmetic point type with the default byte order.
	 * 
	 * @param[out] data The arithmetic type.
	 * 
	 * @return Reference to the current stream.
	 */
	binary_stream& operator>>(arithmetic auto& data) {
		SAFE_READ(&data, sizeof(data), *this);
		endian::storage_out(data, byte_order);
		return *this;
	}

	/**
	 * @brief Deserialises a POD type.
	 * 
	 * @tparam T The type of the POD object.
	 * @param[out] data The object to hold the result.
	 * 
	 * @note Will not be used for types with user-defined serialisation
	 * functions.
	 * 
	 * @return Reference to the current stream.
	 */
	template<pod T>
	requires (!has_shr_override<T, binary_stream> && !arithmetic<T>)
	binary_stream& operator>>(T& data) {
		SAFE_READ(&data, sizeof(data), *this);
		return *this;
	}

	/**
	 * @brief Deserialises an iterable container that was previously written
	 * with a fixed-length prefix.
	 * 
	 * @tparam T The iterable container type.
	 * @param[out] adaptor The container to hold the result.
	 *
	 * @return Reference to the current stream.
	 */
	template<is_iterable T>
	binary_stream& operator>>(prefixed<T> adaptor) {
		std::uint32_t count = 0;
		*this >> endian::le(count);
		read_container(adaptor.str, count);
		return *this;
	}

	/**
	 * @brief Deserialises an iterable container that was previously written
	 * with a variable-length prefix.
	 * 
	 * @tparam T The iterable container type.
	 * @param[out] adaptor The container to hold the result.
	 *  
	 * @return Reference to the current stream.
	 */
	template<is_iterable T>
	binary_stream& operator>>(prefixed_varint<T> adaptor) {
		const auto count = varint_decode<size_type>(*this);
		read_container(adaptor.str, count);
		return *this;
	}

	/**
	 * @brief Read an arithmetic type from the stream.
	 * 
	 * @param[out] dest The variable to hold the result.
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
	 * @param[out] adaptor The destination for the read value.
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
	 * @param[out] dest The std::string to hold the result.
	 * 
	 * @param dest The destination string.
	 */
	void get(std::string& dest) {
		*this >> dest;
	}

	/**
	 * @brief Reads a fixed-length string from the stream.
	 * 
	 * @param[out] dest The std::string to hold the result.
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
	 * @param[out] dest The destination buffer.
	 * @param count The number of elements to be read to the destination.
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
	 * @param[out] begin The beginning iterator.
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
	 * @param[out] dest A contiguous range into which the data should be read.
	 */
	template<std::ranges::contiguous_range range>
	void get(range& dest) {
		const auto read_size = dest.size() * sizeof(typename range::value_type);
		SAFE_READ(dest.data(), read_size, void());
	}

	/**
	 * @brief Skip over a number of bytes.
	 *
	 * Skips over a number of bytes from the stream. This should be used
	 * if the stream holds data that you don't care about but don't want
	 * to have to read it to another buffer to access data beyond it.
	 * 
	 * @param length The number of bytes to skip.
	 */
	void skip(const size_type length) {
		STREAM_READ_BOUNDS_ENFORCE(length, void());
		buffer_.skip(length);
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
	 * @brief Returns the size of the underlying buffer.
	 * 
	 * @note The value returned may be higher than the total number of bytes
	 * that can be read from this stream, if a read limit was set during
	 * construction. Use read_max() to determine how many bytes can be
	 * read from this stream.
	 * 
	 * @return The size of the underlying buffer.
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
			assert(total_read_ < read_limit_);
			return read_limit_ - total_read_;
		} else {
			return buffer_.size();
		}
	}

	/**
	 * @brief Determine whether the stream is in a usable state.
	 * 
	 * @return true if no errors have occurred.
	 */
	bool good() const {
		return state_ == stream_state::ok;
	}

	/**
	 * @brief Resets the stream state back to a good state, allowing
	 * it to be used for streaming operations again. Has no effect
	 * if the stream has not errored.
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

#undef SAFE_READ
#undef STREAM_READ_BOUNDS_ENFORCE

} // hexi

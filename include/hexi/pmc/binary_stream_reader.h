//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/pmc/stream_base.h>
#include <hexi/pmc/buffer_read.h>
#include <hexi/concepts.h>
#include <hexi/endian.h>
#include <hexi/exception.h>
#include <hexi/shared.h>
#include <hexi/stream_adaptors.h>
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

	template<typename container_type, typename count_type>
	void read_container(container_type& container, const count_type count) {
		using cvalue_type = typename container_type::value_type;

		container.clear();

		if constexpr(memcpy_read<container_type, binary_stream_reader>) {
			container.resize(count);

			const auto bytes = count * sizeof(cvalue_type);
			read(container.data(), bytes);
		} else {
			for(count_type i = 0; i < count; ++i) {
				cvalue_type value;
				*this >> value;
				container.emplace_back(std::move(value));
			}
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
	requires has_deserialise<T, binary_stream_reader>
	binary_stream_reader& operator>>(T& data) {
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
	
	/**
	 * @brief Deserialises a string that was previously written with a
	 * variable-length prefix.
	 * 
	 * @param[out] adaptor std::string to hold the result.
	 * 
	 * @return Reference to the current stream.
	 */
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

	/**
	 * @brief Deserialises a string that was previously written with a
	 * null terminator.
	 * 
	 * @param[out] adaptor std::string to hold the result.
	 * 
	 * @return Reference to the current stream.
	 */
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

	/**
	 * @brief Deserialises a string that was previously written with a
	 * fixed-length prefix.
	 * 
	 * @param[out] data std::string_view to hold the result.
	 * 
	 * @note The string_view's lifetime is tied to that of the underlying
	 * buffer. You should not make any further writes to the buffer
	 * unless you know it will not cause the data to be overwritten (i.e.
	 * buffer adaptor with optimise disabled).
	 * 
	 * @return Reference to the current stream.
	 */
	binary_stream_reader& operator>>(std::string& data) {
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
	binary_stream_reader& operator>>(has_shr_override<binary_stream_reader> auto&& data) {
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
	binary_stream_reader& operator>>(endian_func adaptor) {
		read(&adaptor.value, sizeof(adaptor.value));
		adaptor.value = adaptor.from();
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
	requires (!has_shr_override<T, binary_stream_reader>)
	binary_stream_reader& operator>>(T& data) {
		read(&data, sizeof(data));
		return *this;
	}

	/**
	 * @brief Deserialises a POD type.
	 * 
	 * @param[out] data The object to hold the result.
	 * 
	 * @note Will be used for types without user-defined serialisation
	 * functions.
	 * 
	 * @return Reference to the current stream.
	 */
	binary_stream_reader& operator>>(pod auto& data) {
		read(&data, sizeof(data));
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
	binary_stream_reader& operator>>(prefixed<T> adaptor) {
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
	binary_stream_reader& operator>>(prefixed_varint<T> adaptor) {
		const auto count = varint_decode<std::size_t>(*this);
		read_container(adaptor.str, count);
		return *this;
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
	 * @param[out] dest The destination buffer.
	 * @param count The number of elements to be read into the destination.
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
		read(dest.data(), read_size);
	}

	/**
	 * @brief Read an arithmetic type from the stream, allowing for endian
	 * conversion.
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
	 * @param[out] dest The variable to hold the result.
	 */
	void get(arithmetic auto& dest) {
		read(&dest, sizeof(dest));
	}

	/**
	 * @brief Read an arithmetic type from the stream, allowing for endian
	 * conversion.
	 * 
	 * @param[out] adaptor The destination for the read value.
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
	 * @brief Skip over a number of bytes.
	 *
	 * Skips over a number of bytes from the stream. This should be used
	 * if the stream holds data that you don't care about but don't want
	 * to have to read it to another buffer to access data beyond it.
	 * 
	 * @param length The number of bytes to skip.
	 */
	void skip(std::size_t length) {
		enforce_read_bounds(length);
		buffer_.skip(length);
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
			assert(total_read_ < read_limit_);
			return read_limit_ - total_read_;
		} else {
			return buffer_.size();
		}
	}

	/** 
	 * @brief Get a pointer to the buffer read interface.
	 *
	 * @return Pointer to the buffer read interface. 
	 */
	buffer_read* buffer() {
		return &buffer_;
	}

	/** 
	 * @brief Get a pointer to the buffer read interface.
	 *
	 * @return Pointer to the buffer read interface. 
	 */
	const buffer_read* buffer() const {
		return &buffer_;
	}
};

} // pmc, hexi

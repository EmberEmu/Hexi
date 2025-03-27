//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/exception.h>
#include <hexi/shared.h>
#include <hexi/concepts.h>
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

	file_buffer(file_buffer&& rhs) noexcept {
		file_ = rhs.file_;
		read_ = rhs.read_;
		write_ = rhs.write_;
		error_ = rhs.error_;
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
			throw buffer_underrun(length, read_, size());
		}

		if(std::fseek(file_, read_, SEEK_SET)) {
			error_ = true;
			return;
		}
		
		if(std::fread(destination, length, 1, file_) != 1) {
			error_ = true;
			return;
		}

		if(std::fseek(file_, read_, SEEK_SET)) {
			error_ = true;
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

		for(std::size_t i = 0u, j = size(); i < j; ++i) {
			if(std::fread(&buffer, sizeof(value_type), 1, file_) != 1) {
				error_ = true;
				return npos;
			}

			if(buffer == val) {
				if(std::fseek(file_, read_, SEEK_SET)) {
					error_ = true;
					return npos;
				}

				return i;
			}
		}

		if(std::fseek(file_, read_, SEEK_SET)) {
			error_ = true;
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

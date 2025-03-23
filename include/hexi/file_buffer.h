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
		}

		write_ = std::ftell(file_);

		if(write_ == -1) {
			error_ = true;
		}
	}

	~file_buffer() {
		close();
	}

	void close() {
		if(file_) {
			std::fclose(file_);
			file_ = nullptr;
		}
	}

	template<typename T>
	void read(T* destination) {
		read(destination, sizeof(T));
	}

	void read(void* destination, size_type length) {
		if(error_) {
			return;
		}

		if(std::fseek(file_, read_, 0)) {
			error_ = true;
			return;
		}

		std::fread(destination, length, 1, file_);
		read_ += length;
	}

	template<typename T>
	void copy(T* destination) {
		copy(destination, sizeof(T));
	}

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

	size_type find_first_of(value_type val) noexcept {
		if(error_) {
			return npos;
		}

		if(std::fseek(file_, read_, SEEK_SET)) {
			error_ = true;
			return npos;
		}

		value_type buffer{};

		for(std::size_t i = 0u; i < size(); ++i) {
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

	void skip(const size_type length) {
		read_ += length;
	}

	void advance_write(size_type bytes) {
		write_ += bytes;
	}

	[[nodiscard]]
	bool empty() const {
		return write_ == read_;
	}

	constexpr static bool can_write_seek() {
		return std::is_same_v<seeking, supported>;
	}

	void write(const auto& source) {
		write(&source, sizeof(source));
	}

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


	size_type size() const {
		return write_ - read_;
	}

	FILE* handle() {
		return file_;
	}

	const FILE* handle() const {
		return file_;
	}

	bool error() const {
		return error_;
	}

	operator bool() const {
		return !error();
	}
};

} // hexi

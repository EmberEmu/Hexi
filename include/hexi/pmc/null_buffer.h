//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/pmc/buffer_write.h>
#include <hexi/shared.h>
#include <hexi/exception.h>
#include <cstddef>

namespace hexi::pmc {

class null_buffer final : public buffer_write {
public:
	using size_type       = std::size_t;
	using value_type      = std::byte;
	using contiguous      = is_contiguous;
	using seeking         = unsupported;

	void write(const auto& elem) {}
	void write(const void* source, size_type length) override {};
	void read(auto* elem) {}
	void read(void* destination, size_type length) {};
	void copy(auto* elem) const {}
	void copy(void* destination, size_type length) const {};
	void reserve(const size_type length) override {};
	size_type size() const override{ return 0; };
	[[nodiscard]] bool empty() const override { return true; };
	bool can_write_seek() const override { return false; }

	void write_seek(const buffer_seek direction, const std::size_t offset) override {
		throw exception("Don't do this on a null_buffer"); 
	};
};

} // pmc, hexi

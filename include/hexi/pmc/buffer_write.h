//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/pmc/buffer_base.h>
#include <hexi/shared.h>
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

//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/pmr/buffer_read.h>
#include <hexi/pmr/buffer_write.h>

namespace hexi::pmr {

class buffer : public buffer_read, public buffer_write {
public:
	using value_type = std::byte;

	using buffer_read::operator[];

	virtual std::byte& operator[](const std::size_t index) = 0;
	virtual ~buffer() = default;
};

} // pmr, hexi

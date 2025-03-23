//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/pmr/buffer_base.h>
#include <cstddef>

namespace hexi::pmr {

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

} // pmr, hexi

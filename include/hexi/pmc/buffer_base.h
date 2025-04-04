//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <cstddef>

namespace hexi::pmc {

class buffer_base {
protected:
	std::size_t read_ = 0;
	std::size_t write_ = 0;

public:
	virtual std::size_t size() const = 0;
	virtual bool empty() const = 0;
	virtual ~buffer_base() = default;
};

} // pmc, hexi

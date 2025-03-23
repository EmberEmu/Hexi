//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/pmc/buffer_base.h>
#include <hexi/shared.h>

namespace hexi::pmc {

class stream_base {
	buffer_base& buffer_;

public:
	explicit stream_base(buffer_base& buffer) : buffer_(buffer) { }

	std::size_t size() const {
		return buffer_.size();
	}

	[[nodiscard]]
	bool empty() const {
		return buffer_.empty();
	}

	virtual ~stream_base() = default;
};

} // pmc, hexi

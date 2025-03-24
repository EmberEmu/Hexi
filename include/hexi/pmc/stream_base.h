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
	stream_state state_;

protected:
	void set_state(stream_state state) {
		state_ = state;
	}

public:
	explicit stream_base(buffer_base& buffer)
		: buffer_(buffer),
		  state_(stream_state::ok) { }

	std::size_t size() const {
		return buffer_.size();
	}

	[[nodiscard]]
	bool empty() const {
		return buffer_.empty();
	}

	stream_state state() const {
		return state_;
	}

	bool good() const {
		return state() == stream_state::ok;
	}

	void clear_state() {
		set_state(stream_state::ok);
	}

	operator bool() const {
		return good();
	}

	void set_error_state() {
		set_state(stream_state::user_defined_err);
	}

	virtual ~stream_base() = default;
};

} // pmc, hexi

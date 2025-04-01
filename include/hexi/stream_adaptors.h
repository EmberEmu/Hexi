//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <utility>

namespace hexi {

template<typename stream_type>
class stream_read_adaptor final {
	stream_type& _stream;

public:
	stream_read_adaptor(stream_type& stream) 
		: _stream(stream) {}

	void operator&(auto&& arg) {
		_stream >> arg;
	}

	template<typename ...Ts>
	void operator()(Ts&&... args) {
		(_stream >> ... >> args);
	}

	template<typename ...Ts>
	void forward(Ts&&... args) {
		_stream.get(std::forward<Ts>(args)...);
	}
};

template<typename stream_type>
class stream_write_adaptor final {
	stream_type& _stream;

public:
	stream_write_adaptor(stream_type& stream) 
		: _stream(stream) {}

	void operator&(auto&& arg) {
		_stream << arg;
	}

	template<typename ...Ts>
	void operator()(Ts&&... args) {
		(_stream << ... << args);
	}

	template<typename ...Ts>
	void forward(Ts&&... args) {
		_stream.put(std::forward<Ts>(args)...);
	}
};

} // hexi
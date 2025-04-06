//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/pmc/binary_stream_reader.h>
#include <hexi/pmc/binary_stream_writer.h>
#include <hexi/pmc/stream_base.h>
#include <hexi/pmc/buffer.h>
#include <cstddef>

namespace hexi::pmc {

class binary_stream final : public binary_stream_reader, public binary_stream_writer {
public:
	explicit binary_stream(hexi::pmc::buffer& source, std::size_t read_limit = 0)
		: stream_base(source),
		  binary_stream_reader(source, read_limit),
		  binary_stream_writer(source) {}

	explicit binary_stream(hexi::pmc::buffer& source, hexi::no_throw_t, std::size_t read_limit = 0)
		: stream_base(source),
		  binary_stream_reader(source, no_throw, read_limit),
		  binary_stream_writer(source, no_throw) {}

	~binary_stream() override = default;
};

} // pmc, hexi

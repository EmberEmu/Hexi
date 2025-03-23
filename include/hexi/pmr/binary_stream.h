//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/pmr/binary_stream_reader.h>
#include <hexi/pmr/binary_stream_writer.h>
#include <hexi/pmr/stream_base.h>
#include <hexi/pmr/buffer.h>
#include <cstddef>

namespace hexi::pmr {

class binary_stream final : public binary_stream_reader, public binary_stream_writer {
public:
	explicit binary_stream(hexi::pmr::buffer& source, std::size_t read_limit = 0)
		: stream_base(source),
		  binary_stream_reader(source, read_limit),
		  binary_stream_writer(source) {}

	~binary_stream() override = default;
};

} // pmr, hexi

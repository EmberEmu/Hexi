//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <cstddef>

namespace hexi {

template<typename buf_type, typename exception_tag>
class binary_stream;

template<typename buf_type, bool space_optimise = true>
class buffer_adaptor;

template<typename buf_type>
class buffer_sequence;

template<decltype(auto) block_size, typename storage_value_type, typename allocator>
class dynamic_buffer;

class file_buffer;

template<typename storage_type, std::size_t buf_size>
class static_buffer;

namespace pmc {

class binary_stream;
class binary_stream_reader;
class binary_stream_writer;
class stream_base;
class null_buffer;

template<typename buf_type, bool allow_optimise  = true>
class buffer_adaptor;

template<typename buf_type>
class buffer_read_adaptor;

template<typename buf_type>
class buffer_write_adaptor;

} // pmc

} // hexi
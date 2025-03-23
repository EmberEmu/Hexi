//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/binary_stream.h>
#include <hexi/buffer_adaptor.h>
#include <hexi/buffer_sequence.h>
#include <hexi/concepts.h>
#include <hexi/dynamic_buffer.h>
#include <hexi/dynamic_tls_buffer.h>
#include <hexi/exception.h>
#include <hexi/file_buffer.h>
#include <hexi/shared.h>
#include <hexi/static_buffer.h>
#include <hexi/allocators/block_allocator.h>
#include <hexi/allocators/default_allocator.h>
#include <hexi/allocators/tls_block_allocator.h>
#include <hexi/detail/intrusive_storage.h>
#include <hexi/pmc/binary_stream.h>
#include <hexi/pmc/binary_stream_reader.h>
#include <hexi/pmc/binary_stream_writer.h>
#include <hexi/pmc/buffer.h>
#include <hexi/pmc/buffer_adaptor.h>
#include <hexi/pmc/buffer_base.h>
#include <hexi/pmc/buffer_read.h>
#include <hexi/pmc/buffer_read_adaptor.h>
#include <hexi/pmc/buffer_write.h>
#include <hexi/pmc/buffer_write_adaptor.h>
#include <hexi/pmc/null_buffer.h>
#include <hexi/pmc/stream_base.h>
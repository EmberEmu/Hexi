#  _               _ 
# | |__   _____  _(_)
# | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
# | | | |  __/>  <| | Version 1.0
# |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

set(HEADERS
    hexi/hexi.h
    hexi/exception.h
    hexi/endian.h
    hexi/dynamic_buffer.h
    hexi/dynamic_tls_buffer.h
    hexi/shared.h
    hexi/exception.h
    hexi/buffer_adaptor.h
    hexi/buffer_sequence.h
    hexi/binary_stream.h
    hexi/static_buffer.h
    hexi/concepts.h
    hexi/detail/intrusive_storage.h
    hexi/file_buffer.h
    hexi/null_buffer.h
    hexi/stream_adaptors.h
    hexi/pmc/buffer_base.h
    hexi/pmc/buffer_read.h
    hexi/pmc/buffer_write.h
    hexi/pmc/buffer.h
    hexi/pmc/binary_stream.h
    hexi/pmc/binary_stream_reader.h
    hexi/pmc/binary_stream_writer.h
    hexi/pmc/stream_base.h
    hexi/pmc/buffer_adaptor.h
    hexi/pmc/buffer_read_adaptor.h
    hexi/pmc/buffer_write_adaptor.h
    hexi/allocators/default_allocator.h
    hexi/allocators/tls_block_allocator.h
    hexi/allocators/block_allocator.h
)

add_library(${HEXI_PROJECT_NAME} INTERFACE ${HEADERS})
target_include_directories(${HEXI_PROJECT_NAME} INTERFACE ../)

#  _               _ 
# | |__   _____  _(_)
# | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
# | | | |  __/>  <| | Version 1.0
# |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

set(HEADERS
    hexi/exception.h
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
    hexi/pmr/buffer_base.h
    hexi/pmr/buffer_read.h
    hexi/pmr/buffer_write.h
    hexi/pmr/buffer.h
    hexi/pmr/null_buffer.h
    hexi/pmr/binary_stream.h
    hexi/pmr/binary_stream_reader.h
    hexi/pmr/binary_stream_writer.h
    hexi/pmr/stream_base.h
    hexi/pmr/buffer_adaptor.h
    hexi/pmr/buffer_read_adaptor.h
    hexi/pmr/buffer_write_adaptor.h
    hexi/allocators/default_allocator.h
    hexi/allocators/tls_block_allocator.h
    hexi/allocators/block_allocator.h
)

add_library(${CMAKE_PROJECT_NAME} INTERFACE ${HEADERS})
target_include_directories(${CMAKE_PROJECT_NAME} INTERFACE ../)

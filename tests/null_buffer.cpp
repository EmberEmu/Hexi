//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#include <hexi/binary_stream.h>
#include <hexi/null_buffer.h>
#include <gtest/gtest.h>
#include <string_view>
#include <cstdint>

TEST(null_buffer, write_size) {
	hexi::null_buffer buffer;
	hexi::binary_stream stream(buffer);

	stream << std::uint8_t(0);
	ASSERT_EQ(stream.total_write(), 1);
	stream << std::uint16_t(0);
	ASSERT_EQ(stream.total_write(), 3);
	stream << std::uint32_t(0);
	ASSERT_EQ(stream.total_write(), 7);
	stream << std::uint64_t(0);
	ASSERT_EQ(stream.total_write(), 15);

	std::string_view str { "A library serves no purpose unless someone is using it." };
	stream << hexi::null_terminated(str);
	ASSERT_EQ(stream.total_write(), 71);
	ASSERT_TRUE(buffer.empty());
}

TEST(null_buffer, write_seek) {
	hexi::null_buffer buffer;
	hexi::binary_stream stream(buffer);
	ASSERT_THROW(buffer.write_seek(hexi::buffer_seek::sk_absolute, 0), hexi::exception);
}
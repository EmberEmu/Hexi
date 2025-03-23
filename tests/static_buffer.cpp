//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#include <hexi/static_buffer.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <span>
#include <string_view>
#include <vector>
#include <cstdint>

using namespace std::literals;

TEST(static_buffer, initial_empty) {
	hexi::static_buffer<char, 1> buffer;
	ASSERT_TRUE(buffer.empty());
	ASSERT_EQ(buffer.size(), 0);
}

TEST(static_buffer, initial_not_empty) {
	hexi::static_buffer<char, 2> buffer { '1', '2' };
	ASSERT_FALSE(buffer.empty());
	ASSERT_EQ(buffer.size(), 2);
	ASSERT_EQ(buffer[0], '1');
	ASSERT_EQ(buffer[1], '2');
}

TEST(static_buffer, empty) {
	hexi::static_buffer<char, 1> buffer;
	ASSERT_TRUE(buffer.empty());
	char val = '\0';
	buffer.write(&val, 1);
	ASSERT_FALSE(buffer.empty());
}

TEST(static_buffer, read_one) {
	hexi::static_buffer<char, 3> buffer { '1', '2', '3' };
	char value = 0;
	buffer.read(&value, 1);
	ASSERT_EQ(buffer.size(), 2);
	ASSERT_EQ(value, '1');
}

TEST(static_buffer, read_all) {
	hexi::static_buffer<char, 3> buffer { '1', '2', '3' };
	std::array<char, 3> expected{ '1', '2', '3' };
	std::array<char, 3> values{};
	buffer.read(values.data(), values.size());
	ASSERT_TRUE(std::ranges::equal(values, expected));
}

TEST(static_buffer, single_read_skip) {
	hexi::static_buffer<char, 3> buffer { '1', '2', '3' };
	std::uint8_t value = 0;
	buffer.skip(1);
	buffer.read(&value, 1);
	ASSERT_EQ(buffer.size(), 1);
	ASSERT_EQ(buffer[0], '3');
}

TEST(static_buffer, multiskip_read) {
	hexi::static_buffer<char, 6> buffer { '1', '2', '3', '4', '5', '6' };
	std::uint8_t value = 0;
	buffer.skip(5);
	buffer.read(&value, 1);
	ASSERT_TRUE(buffer.empty());
	ASSERT_EQ(value, '6');
}

TEST(static_buffer, write) {
	hexi::static_buffer<std::uint8_t, 6> buffer;
	const std::array<std::uint8_t, 6> values { 1, 2, 3, 4, 5, 6 };
	buffer.write(values.data(), values.size());
	ASSERT_EQ(buffer.size(), values.size());
	ASSERT_TRUE(std::ranges::equal(values, buffer));
}

TEST(static_buffer, can_write_seek) {
	hexi::static_buffer<char, 1> buffer;
	ASSERT_TRUE(buffer.can_write_seek());
}

TEST(static_buffer, write_seek_back) {
	hexi::static_buffer<char, 3> buffer { '1', '2', '3' };
	std::array<char, 2> values { '5', '6' };
	buffer.write_seek(hexi::buffer_seek::sk_backward, 2);
	buffer.write(values.data(), values.size());
	std::array<char, 3> expected { '1', '5', '6' };
	ASSERT_TRUE(std::ranges::equal(expected, buffer));
}

TEST(static_buffer, write_seek_start) {
	hexi::static_buffer<char, 3> buffer { '1', '2', '3' };
	std::array<char, 3> values { '4', '5', '6' };
	buffer.write_seek(hexi::buffer_seek::sk_absolute, 0);
	buffer.write(values.data(), values.size());
	ASSERT_EQ(buffer.size(), values.size());
	ASSERT_TRUE(std::ranges::equal(buffer, values));
}

TEST(static_buffer, read_ptr) {
	hexi::static_buffer<char, 3> buffer { '1', '2', '3' };
	auto ptr = buffer.read_ptr();
	ASSERT_EQ(*ptr, buffer[0]);
	buffer.skip(1);
	ptr = buffer.read_ptr();
	ASSERT_EQ(*ptr, buffer[0]);
	buffer.skip(1);
	ptr = buffer.read_ptr();
	ASSERT_EQ(*ptr, buffer[0]);
}

TEST(static_buffer, subscript) {
	hexi::static_buffer<char, 3> buffer { '1', '2', '3' };
	ASSERT_EQ(buffer[0], '1');
	ASSERT_EQ(buffer[1], '2');
	ASSERT_EQ(buffer[2], '3');
	buffer[0] = '4';
	ASSERT_EQ(buffer[0], '4');
	buffer[0] = '5';
	ASSERT_EQ(buffer[0], '5');
	ASSERT_EQ(buffer[0], buffer[0]);
}

TEST(static_buffer, find_first_of) {
	hexi::static_buffer<char, 64> buffer;
	const auto str = "The quick brown fox jumped over the lazy dog"sv;
	buffer.write(str.data(), str.size());
	auto pos = buffer.find_first_of('\0');
	ASSERT_EQ(pos, buffer.npos); // direct string write is not terminated
	pos = buffer.find_first_of('g');
	ASSERT_EQ(pos, 43);
	pos = buffer.find_first_of('T');
	ASSERT_EQ(pos, 0);
	pos = buffer.find_first_of('t');
	ASSERT_EQ(pos, 32);
}

TEST(static_buffer, advance_write) {
	hexi::static_buffer<char, 3> buffer { 'a', 'b', 'c' };
	buffer.write_seek(hexi::buffer_seek::sk_absolute, 0);
	const char val = 'd';
	buffer.advance_write(1);
	buffer.write(&val, 1);
	ASSERT_EQ(buffer[1], val);
}

TEST(static_buffer, defragment) {
	hexi::static_buffer<char, 3> buffer { 'a', 'b', 'c' };
	ASSERT_EQ(buffer.free(), 0);
	char value = 0;
	buffer.read(&value, sizeof(value));
	ASSERT_EQ(value, 'a');
	ASSERT_EQ(buffer.free(), 0);
	buffer.defragment();
	ASSERT_EQ(buffer.free(), 1);
	buffer.copy(&value, sizeof(value));
	ASSERT_EQ(value, 'b');
	ASSERT_EQ(buffer[0], 'b');
	ASSERT_EQ(*buffer.read_ptr(), 'b');
	buffer.read(&value, sizeof(value));
	ASSERT_EQ(value, 'b');
	buffer.read(&value, sizeof(value));
	ASSERT_EQ(value, 'c');
	ASSERT_THROW(buffer.read(&value, sizeof(value)), hexi::buffer_underrun);
}

TEST(static_buffer, free_buff) {
	hexi::static_buffer<char, 3> buffer;
	ASSERT_EQ(buffer.free(), 3);
	char value = 0;
	buffer.write(&value, sizeof(value));
	ASSERT_EQ(buffer.free(), 2);
	buffer.write(&value, sizeof(value));
	ASSERT_EQ(buffer.free(), 1);
	buffer.write(&value, sizeof(value));
	ASSERT_EQ(buffer.free(), 0);
	ASSERT_THROW(buffer.write(&value, sizeof(value)), hexi::buffer_overflow);
}
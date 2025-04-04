//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#include <hexi/buffer_adaptor.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <span>
#include <string_view>
#include <vector>
#include <cstdint>

using namespace std::literals;

TEST(buffer_adaptor, size_empty_initial) {
	std::array<std::uint8_t, 0> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	ASSERT_EQ(adaptor.size(), 0);
}

TEST(buffer_adaptor, empty) {
	std::vector<std::uint8_t> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	ASSERT_TRUE(adaptor.empty());
	buffer.emplace_back(1);
	adaptor.advance_write(1);
	ASSERT_FALSE(adaptor.empty());
}

TEST(buffer_adaptor, size_populated_initial) {
	std::array<std::uint8_t, 1> buffer { 1 };
	hexi::buffer_adaptor adaptor(buffer);
	ASSERT_EQ(adaptor.size(), buffer.size());
}

TEST(buffer_adaptor, resize_match) {
	std::vector<std::uint8_t> buffer { 1, 2, 3, 4, 5 };
	hexi::buffer_adaptor adaptor(buffer);
	ASSERT_EQ(adaptor.size(), buffer.size());
	buffer.emplace_back(6);
	adaptor.advance_write(1);
	ASSERT_EQ(adaptor.size(), buffer.size());
}

TEST(buffer_adaptor, read_one) {
	std::array<std::uint8_t, 3> buffer { 1, 2, 3 };
	hexi::buffer_adaptor adaptor(buffer);
	std::uint8_t value = 0;
	adaptor.read(&value, 1);
	ASSERT_EQ(adaptor.size(), buffer.size() - 1);
	ASSERT_EQ(value, 1);
}

TEST(buffer_adaptor, read_all) {
	std::array<std::uint8_t, 3> buffer { 1, 2, 3 };
	hexi::buffer_adaptor adaptor(buffer);
	std::array<std::uint8_t, 3> values{};
	adaptor.read(&values, values.size());
	ASSERT_TRUE(std::ranges::equal(buffer, values));
}

TEST(buffer_adaptor, single_skip_read) {
	std::array<std::uint8_t, 3> buffer { 1, 2, 3 };
	hexi::buffer_adaptor adaptor(buffer);
	std::uint8_t value = 0;
	adaptor.skip(1);
	adaptor.read(&value, 1);
	ASSERT_EQ(adaptor.size(), 1);
	ASSERT_EQ(value, buffer[1]);
}

TEST(buffer_adaptor, multiskip_read) {
	std::array<std::uint8_t, 6> buffer { 1, 2, 3, 4, 5, 6 };
	hexi::buffer_adaptor<decltype(buffer), false> adaptor(buffer);
	std::uint8_t value = 0;
	adaptor.skip(5);
	adaptor.read(&value, 1);
	ASSERT_TRUE(adaptor.empty());
	ASSERT_EQ(value, buffer[5]);
}

TEST(buffer_adaptor, write) {
	std::vector<std::uint8_t> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	std::array<std::uint8_t, 6> values { 1, 2, 3, 4, 5, 6 };
	adaptor.write(values.data(), values.size());
	ASSERT_EQ(adaptor.size(), values.size());
	ASSERT_EQ(buffer.size(), values.size());
	ASSERT_TRUE(std::ranges::equal(values, buffer));
	const auto size = adaptor.size();
	adaptor.write('\0');
	ASSERT_EQ(adaptor.size(), size + 1);
}

TEST(buffer_adaptor, write_append) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	hexi::buffer_adaptor adaptor(buffer);
	std::array<std::uint8_t, 3> values { 4, 5, 6 };
	adaptor.write(values.data(), values.size());
	ASSERT_EQ(buffer.size(), 6);
	ASSERT_EQ(adaptor.size(), buffer.size());
	std::array<std::uint8_t, 6> expected { 1, 2, 3, 4, 5, 6 };
	ASSERT_TRUE(std::ranges::equal(expected, buffer));
}

TEST(buffer_adaptor, can_write_seek) {
	std::vector<std::uint8_t> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	ASSERT_TRUE(adaptor.can_write_seek());
}

TEST(buffer_adaptor, write_seek_back) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	hexi::buffer_adaptor adaptor(buffer);
	std::array<std::uint8_t, 3> values { 4, 5, 6 };
	adaptor.write_seek(hexi::buffer_seek::sk_backward, 2);
	adaptor.write(values.data(), values.size());
	ASSERT_EQ(buffer.size(), 4);
	ASSERT_EQ(adaptor.size(), buffer.size());
	std::array<std::uint8_t, 4> expected { 1, 4, 5, 6 };
	ASSERT_TRUE(std::ranges::equal(expected, buffer));
}

TEST(buffer_adaptor, write_seek_start) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	hexi::buffer_adaptor adaptor(buffer);
	std::array<std::uint8_t, 3> values { 4, 5, 6 };
	adaptor.write_seek(hexi::buffer_seek::sk_absolute, 0);
	adaptor.write(values.data(), values.size());
	ASSERT_EQ(buffer.size(), values.size());
	ASSERT_EQ(adaptor.size(), buffer.size());
	ASSERT_TRUE(std::ranges::equal(buffer, values));
}

TEST(buffer_adaptor, read_ptr) {
	std::array<std::uint8_t, 3> buffer { 1, 2, 3 };
	hexi::buffer_adaptor adaptor(buffer);
	auto ptr = adaptor.read_ptr();
	ASSERT_EQ(*ptr, buffer[0]);
	adaptor.skip(1);
	ptr = adaptor.read_ptr();
	ASSERT_EQ(*ptr, buffer[1]);
	adaptor.skip(1);
	ptr = adaptor.read_ptr();
	ASSERT_EQ(*ptr, buffer[2]);
}

TEST(buffer_adaptor, subscript) {
	std::array<std::uint8_t, 3> buffer { 1, 2, 3 };
	hexi::buffer_adaptor adaptor(buffer);
	ASSERT_EQ(adaptor[0], 1);
	ASSERT_EQ(adaptor[1], 2);
	ASSERT_EQ(adaptor[2], 3);
	buffer[0] = 4;
	ASSERT_EQ(adaptor[0], 4);
	adaptor[0] = 5;
	ASSERT_EQ(adaptor[0], 5);
	ASSERT_EQ(adaptor[0], buffer[0]);
}

TEST(buffer_adaptor, find_first_of) {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	const auto str = "The quick brown fox jumped over the lazy dog"sv;
	adaptor.write(str.data(), str.size());
	auto pos = adaptor.find_first_of('\0');
	ASSERT_EQ(pos, adaptor.npos); // direct string write is not terminated
	pos = adaptor.find_first_of('g');
	ASSERT_EQ(pos, 43);
	pos = adaptor.find_first_of('T');
	ASSERT_EQ(pos, 0);
	pos = adaptor.find_first_of('t');
	ASSERT_EQ(pos, 32);
}

// test optimised write() for buffers supporting resize_and_overwrite
TEST(buffer_adaptor, string_buffer) {
	std::string buffer;
	hexi::buffer_adaptor adaptor(buffer);
	const auto str = "The quick brown fox jumped over the lazy dog"sv;
	adaptor.write(str.data(), str.size());
	ASSERT_EQ(buffer, str);
}
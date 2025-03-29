//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#include <hexi/pmc/buffer_adaptor.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <span>
#include <string_view>
#include <vector>
#include <cstdint>

using namespace std::literals;

TEST(buffer_adaptor_pmc, size_empty_initial) {
	std::vector<std::uint8_t> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	ASSERT_EQ(adaptor.size(), 0);
}

TEST(buffer_adaptor_pmc, empty) {
	std::vector<std::uint8_t> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	ASSERT_TRUE(adaptor.empty());
	buffer.emplace_back(1);
	ASSERT_FALSE(adaptor.empty());
}

TEST(buffer_adaptor_pmc, size_populated_initial) {
	std::vector<std::uint8_t> buffer { 1 };
	hexi::pmc::buffer_adaptor adaptor(buffer);
	ASSERT_EQ(adaptor.size(), buffer.size());
}

TEST(buffer_adaptor_pmc, resize_match) {
	std::vector<std::uint8_t> buffer { 1, 2, 3, 4, 5 };
	hexi::pmc::buffer_adaptor adaptor(buffer);
	ASSERT_EQ(adaptor.size(), buffer.size());
	buffer.emplace_back(6);
	ASSERT_EQ(adaptor.size(), buffer.size());
}

TEST(buffer_adaptor_pmc, read_one) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	hexi::pmc::buffer_adaptor adaptor(buffer);
	std::uint8_t value = 0;
	adaptor.read(&value, 1);
	ASSERT_EQ(adaptor.size(), buffer.size() - 1);
	ASSERT_EQ(value, 1);
}

TEST(buffer_adaptor_pmc, read_all) {
	// unless buffer reuse optimisation is disabled, the original buffer
	// will be emptied when fully read, so we need a copy for testing
	// the output
	std::array<std::uint8_t, 3> expected { 1, 2, 3 };
	std::vector<std::uint8_t> buffer(expected.begin(), expected.end());
	hexi::pmc::buffer_adaptor adaptor(buffer);
	std::array<std::uint8_t, 3> values{};
	adaptor.read(values.data(), values.size());
	ASSERT_TRUE(std::ranges::equal(expected, values));
}

TEST(buffer_adaptor_pmc, single_skip_read) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	hexi::pmc::buffer_adaptor adaptor(buffer);
	std::uint8_t value = 0;
	adaptor.skip(1);
	adaptor.read(&value, 1);
	ASSERT_EQ(adaptor.size(), 1);
	ASSERT_EQ(value, buffer[1]);
}

TEST(buffer_adaptor_pmc, multiskip_read) {
	std::vector<std::uint8_t> buffer { 1, 2, 3, 4, 5, 6 };
	hexi::pmc::buffer_adaptor<decltype(buffer), false> adaptor(buffer);
	std::uint8_t value = 0;
	adaptor.skip(5);
	adaptor.read(&value, 1);
	ASSERT_TRUE(adaptor.empty());
	ASSERT_EQ(value, buffer[5]);
}

TEST(buffer_adaptor_pmc, write) {
	std::vector<std::uint8_t> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	std::array<std::uint8_t, 6> values { 1, 2, 3, 4, 5, 6 };
	adaptor.write(values.data(), values.size());
	ASSERT_EQ(adaptor.size(), values.size());
	ASSERT_EQ(buffer.size(), values.size());
	ASSERT_TRUE(std::ranges::equal(values, buffer));
	const auto size = adaptor.size();
	adaptor.write('\0');
	ASSERT_EQ(adaptor.size(), size + 1);
}

TEST(buffer_adaptor_pmc, write_append) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	hexi::pmc::buffer_adaptor adaptor(buffer);
	std::array<std::uint8_t, 3> values { 4, 5, 6 };
	adaptor.write(values.data(), values.size());
	ASSERT_EQ(buffer.size(), 6);
	ASSERT_EQ(adaptor.size(), buffer.size());
	std::array<std::uint8_t, 6> expected { 1, 2, 3, 4, 5, 6 };
	ASSERT_TRUE(std::ranges::equal(expected, buffer));
}

TEST(buffer_adaptor_pmc, can_write_seek) {
	std::vector<std::uint8_t> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	ASSERT_TRUE(adaptor.can_write_seek());
}

TEST(buffer_adaptor_pmc, write_seek_back) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	hexi::pmc::buffer_adaptor adaptor(buffer);
	std::array<std::uint8_t, 3> values { 4, 5, 6 };
	adaptor.write_seek(hexi::buffer_seek::sk_backward, 2);
	adaptor.write(values.data(), values.size());
	ASSERT_EQ(buffer.size(), 4);
	ASSERT_EQ(adaptor.size(), buffer.size());
	std::array<std::uint8_t, 4> expected { 1, 4, 5, 6 };
	ASSERT_TRUE(std::ranges::equal(expected, buffer));
}

TEST(buffer_adaptor_pmc, write_seek_start) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	hexi::pmc::buffer_adaptor adaptor(buffer);
	std::array<std::uint8_t, 3> values { 4, 5, 6 };
	adaptor.write_seek(hexi::buffer_seek::sk_absolute, 0);
	adaptor.write(values.data(), values.size());
	ASSERT_EQ(buffer.size(), values.size());
	ASSERT_EQ(adaptor.size(), buffer.size());
	ASSERT_TRUE(std::ranges::equal(buffer, values));
}

TEST(buffer_adaptor_pmc, read_ptr) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	hexi::pmc::buffer_adaptor adaptor(buffer);
	auto ptr = adaptor.read_ptr();
	ASSERT_EQ(*ptr, buffer[0]);
	adaptor.skip(1);
	ptr = adaptor.read_ptr();
	ASSERT_EQ(*ptr, buffer[1]);
	adaptor.skip(1);
	ptr = adaptor.read_ptr();
	ASSERT_EQ(*ptr, buffer[2]);
}

TEST(buffer_adaptor_pmc, subscript) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	hexi::pmc::buffer_adaptor adaptor(buffer);
	ASSERT_EQ(adaptor[0], std::byte(1));
	ASSERT_EQ(adaptor[1], std::byte(2));
	ASSERT_EQ(adaptor[2], std::byte(3));
	buffer[0] = 4;
	ASSERT_EQ(adaptor[0], std::byte(4));
}

TEST(buffer_adaptor_pmc, find_first_of) {
	std::vector<char> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	const auto str = "The quick brown fox jumped over the lazy dog"sv;
	adaptor.write(str.data(), str.size());
	auto pos = adaptor.find_first_of(std::byte('\0'));
	ASSERT_EQ(pos, adaptor.npos); // direct string write is not terminated
	pos = adaptor.find_first_of(std::byte('g'));
	ASSERT_EQ(pos, 43);
	pos = adaptor.find_first_of(std::byte('T'));
	ASSERT_EQ(pos, 0);
	pos = adaptor.find_first_of(std::byte('t'));
	ASSERT_EQ(pos, 32);
}

// test optimised write() for buffers supporting resize_and_overwrite
TEST(buffer_adaptor_pmc, string_buffer) {
	std::string buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	const auto str = "The quick brown fox jumped over the lazy dog"sv;
	adaptor.write(str.data(), str.size());
	ASSERT_EQ(buffer, str);
}
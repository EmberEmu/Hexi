//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#include "final_action.h"
#include "helpers.h"
#include <hexi/file_buffer.h>
#include <hexi/endian.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <cstdio>

TEST(file_buffer, read) {
	std::filesystem::path path("data/filebuffer");
	ASSERT_TRUE(std::filesystem::exists(path));
	
	hexi::file_buffer buffer(path);
	ASSERT_TRUE(buffer) << "File open failed";

	std::uint8_t w = 0;
	std::uint16_t x = 0;
	std::uint32_t y = 0;
	std::uint64_t z = 0;

	std::string_view strcmp { "The quick brown fox jumped over the lazy dog." };
	std::string str_out;
	str_out.resize(strcmp.size());

	buffer.read(&w);
	buffer.read(&x);
	buffer.read(&y);
	buffer.read(&z);
	buffer.read(str_out.data(), str_out.size());

	ASSERT_TRUE(buffer) << "File read error occurred";

	hexi::endian::little_to_native_inplace(x);
	hexi::endian::little_to_native_inplace(y);
	hexi::endian::little_to_native_inplace(z);

	ASSERT_EQ(w, 47) << "Wrong uint8 value";
	ASSERT_EQ(x, 49197) << "Wrong uint16 value";
	ASSERT_EQ(y, 2173709693) << "Wrong uint32 value";
	ASSERT_EQ(z, 1438110846748337907) << "Wrong uint64 value";
	ASSERT_EQ(str_out, strcmp);
}

TEST(file_buffer, write) {
	std::filesystem::path path { "tmp_unittest_file_buffer_write" };

	final_action fa([path] {
		std::filesystem::remove(path);
	});

	// in case previous clean-up failed
	std::filesystem::remove(path);

	hexi::file_buffer buffer(path);
	ASSERT_TRUE(buffer) << "File open failed";

	std::uint8_t w = 47;
	std::uint16_t x = 49197;
	std::uint32_t y = 2173709693;
	std::uint64_t z = 1438110846748337907;
	std::string str { "The quick brown fox jumped over the lazy dog." };

	hexi::endian::native_to_little_inplace(x);
	hexi::endian::native_to_little_inplace(y);
	hexi::endian::native_to_little_inplace(z);

	buffer.write(w);
	buffer.write(x);
	buffer.write(y);
	buffer.write(z);
	buffer.write(str.data(), str.size() + 1); // write terminator
	buffer.flush(); // ensure data has been written before the read

	const auto reference = read_file("data/filebuffer");
	const auto created = read_file(path);
	ASSERT_TRUE(std::ranges::equal(reference, created));
}

TEST(file_buffer, copy) {
	std::filesystem::path path("data/filebuffer");
	ASSERT_TRUE(std::filesystem::exists(path));

	hexi::file_buffer buffer(path);

	std::uint8_t in = 0;
	buffer.copy(&in);
	ASSERT_EQ(in, 47);
	buffer.copy(&in);
	ASSERT_EQ(in, 47);
	buffer.read(&in); // advance to next byte without skip
	ASSERT_EQ(in, 47);
	buffer.copy(&in);
	ASSERT_EQ(in, 45);
}

TEST(file_buffer, skip) {
	std::filesystem::path path("data/filebuffer");
	ASSERT_TRUE(std::filesystem::exists(path));

	hexi::file_buffer buffer(path);

	{
		std::uint8_t in = 0;
		buffer.read(&in);
		ASSERT_EQ(in, 47);
		buffer.skip(1);
		buffer.read(&in);
		ASSERT_EQ(in, 192);
	}

	buffer.skip(4);

	{
		std::uint32_t in = 0;
		buffer.read(&in);
		ASSERT_EQ(in, 403842803);
	}

	ASSERT_TRUE(buffer) << "Read failure";
}

TEST(file_buffer, initial_size) {
	std::filesystem::path path("data/filebuffer");
	ASSERT_TRUE(std::filesystem::exists(path));

	hexi::file_buffer buffer(path);
	ASSERT_TRUE(buffer) << "File open failed";
	ASSERT_EQ(buffer.size(), 61) << "Wrong size";
}

TEST(file_buffer, read_write_interleave) {
	std::filesystem::path path { "tmp_unittest_file_buffer_read_write_mix" };

	final_action fa([path] {
		std::filesystem::remove(path);
	});

	// in case previous clean-up failed
	std::filesystem::remove(path);

	hexi::file_buffer buffer(path);
	ASSERT_TRUE(buffer) << "File open failed";

	{
		std::uint8_t in = 42;
		std::uint8_t out = 0;
		buffer.write(in);
		buffer.read(&out);
		ASSERT_EQ(in, out);
	}

	{
		std::uint16_t in = 64245;
		std::uint16_t out = 0;
		buffer.write(in);
		buffer.read(&out);
		ASSERT_EQ(in, out);
	}

	{
		std::uint32_t in = 80144;
		std::uint32_t out = 0;
		buffer.write(in);
		buffer.read(&out);
		ASSERT_EQ(in, out);
	}

	{
		std::uint64_t in = 1438110846748337907;
		std::uint64_t out = 0;
		buffer.write(in);
		buffer.read(&out);
		ASSERT_EQ(in, out);
	}

	{
		std::uint16_t x_in = 60925;
		std::uint16_t y_in = 1352;
		std::uint16_t x_out = 0;
		std::uint16_t y_out = 0;
		buffer.write(x_in);
		buffer.write(y_in);
		buffer.read(&x_out);
		buffer.read(&y_out);
		ASSERT_EQ(x_in, x_out);
		ASSERT_EQ(y_in, y_out);
	}
}

TEST(file_buffer, find_first_of) {
	std::filesystem::path path("data/filebuffer");

	ASSERT_TRUE(std::filesystem::exists(path));

	hexi::file_buffer buffer(path);
	ASSERT_TRUE(buffer) << "File open failed";

	EXPECT_EQ(buffer.find_first_of(0x2f), 0);
	EXPECT_EQ(buffer.find_first_of(0x20), 18);
	EXPECT_EQ(buffer.find_first_of(0x6f), 27);
	EXPECT_EQ(buffer.find_first_of(0x6a), 35);
	EXPECT_EQ(buffer.find_first_of(0x00), 60);
	EXPECT_EQ(buffer.find_first_of(0xff), hexi::file_buffer::npos);
	ASSERT_TRUE(buffer) << "Error occurred during seeking";
}
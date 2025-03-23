//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#include <hexi/shared.h>
#include <gtest/gtest.h>
#include <array>
#include <cstdint>

using namespace hexi::detail;

TEST(buffer_utility, src_dest_overlap_start) {
	std::array<std::uint8_t, 10> buffer{};
	ASSERT_TRUE(region_overlap(buffer.data(), buffer.size(), buffer.data(), buffer.size()));
}

TEST(buffer_utility, src_dest_overlap_end) {
	std::array<std::uint8_t, 10> buffer{};
	const auto end = buffer.data() + buffer.size();
	ASSERT_TRUE(region_overlap(buffer.data(), buffer.size(), end - 1, 1));
}

TEST(buffer_utility, src_dest_overlap_beyond_end) {
	std::array<std::uint8_t, 10> buffer{};
	const auto end = buffer.data() + buffer.size();
	ASSERT_FALSE(region_overlap(buffer.data(), buffer.size(), end, 1));
}

TEST(buffer_utility, src_dest_overlap_between) {
	std::array<std::uint8_t, 10> buffer{};
	ASSERT_TRUE(region_overlap(buffer.data(), buffer.size(), buffer.data() + (buffer.size() - 5), 1));
}

TEST(buffer_utility, src_dest_overlap_no_overlap) {
	std::array<std::uint8_t, 10> buffer{}, buffer2{};
	ASSERT_FALSE(region_overlap(buffer.data(), buffer.size(), buffer2.data(), buffer2.size()));
}
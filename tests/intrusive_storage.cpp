//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#include <hexi/detail/intrusive_storage.h>
#include <gtest/gtest.h>
#include <array>
#include <string_view>



TEST(intrusive_storage, size) {
	const int iterations = 5;
	hexi::detail::intrusive_storage<sizeof(int) * iterations> buffer;
	int foo = 24221;
	std::size_t written = 0;

	for(int i = 0; i < 5; ++i) {
		written += buffer.write(&foo, sizeof(int));
	}

	ASSERT_EQ(sizeof(int) * iterations, written) << "Number of bytes written is incorrect";
	ASSERT_EQ(sizeof(int) * iterations, buffer.size()) << "Buffer size is incorrect";

	// attempt to exceed capacity - write should return 0
	written = buffer.write(&foo, sizeof(int));
	ASSERT_EQ(0, written) << "Number of bytes written is incorrect";
	ASSERT_EQ(sizeof(int) * iterations, buffer.size()) << "Buffer size is incorrect";
}

TEST(intrusive_storage, read_write_consistency) {
	const char text[] = "The quick brown fox jumps over the lazy dog";
	hexi::detail::intrusive_storage<sizeof(text)> buffer;

	std::size_t written = buffer.write(text, sizeof(text));
	ASSERT_EQ(sizeof(text), written) << "Incorrect write size";

	char text_out[sizeof(text)];

	std::size_t read = buffer.read(text_out, sizeof(text));
	ASSERT_EQ(sizeof(text), read) << "Incorrect read size";

	ASSERT_STREQ(text, text_out) << "Read produced incorrect result";
	ASSERT_EQ(0, buffer.size()) << "Buffer should be empty";
}

TEST(intrusive_storage, skip) {
	const char text[] = "The quick brown fox jumps over the lazy dog";
	hexi::detail::intrusive_storage<sizeof(text)> buffer;

	buffer.write(text, sizeof(text));

	std::array<char, sizeof(text)> text_out{};
	std::size_t skipped = buffer.skip(4);
	ASSERT_EQ(4, skipped) << "Incorrect skip length";
	
	buffer.read(text_out.data(), sizeof(text) - 4);
	ASSERT_STREQ("quick brown fox jumps over the lazy dog", text_out.data())
		<< "Skip/read produced incorrect result";
}

TEST(intrusive_storage, read_write_string_view) {
	hexi::detail::intrusive_storage<128, char> buffer;
	std::string_view str { "The quick brown fox jumped over the lazy dog" };
	buffer.write(str.data(), str.size());
	ASSERT_EQ(str.size(), buffer.size());
	ASSERT_STREQ(str.data(), buffer.read_data());

	std::array<char, 128> out{};
	buffer.read(out.data(), str.size());
	ASSERT_STREQ(str.data(), out.data());
	ASSERT_TRUE(buffer.size() == 0);
}

TEST(intrusive_storage, read_wrie_ints) {
	hexi::detail::intrusive_storage<128, char> buffer;
	std::array<int, 4> in { 42, 1657, 1558, -1563 };
	buffer.write(in.data(), sizeof(in));
	ASSERT_EQ(sizeof(in), buffer.size());

	std::array<int, 4> out {};
	buffer.read(out.data(), sizeof(out));
	ASSERT_EQ(in, out);
	ASSERT_TRUE(buffer.size() == 0);
}

TEST(intrusive_storage, subscript) {
	hexi::detail::intrusive_storage<8, char> buffer;
	std::string_view str { "ABC" };
	buffer.write(str.data(), str.size());
	ASSERT_EQ(str[0], buffer[0]);
	ASSERT_EQ(str[1], buffer[1]);
	ASSERT_EQ(str[2], buffer[2]);
	buffer[0] = 'C';
	buffer[1] = 'D';
	buffer[2] = 'E';
	ASSERT_EQ('C', buffer[0]);
	ASSERT_EQ('D', buffer[1]);
	ASSERT_EQ('E', buffer[2]);
}

TEST(intrusive_storage, advance_write) {
	hexi::detail::intrusive_storage<32, char> buffer;
	constexpr std::string_view str {"A short string"};
	const std::size_t advance = 10;
	buffer.advance_write(advance);
	ASSERT_EQ(buffer.size(), advance);
	buffer.write(str.data(), str.size() + 1);
	ASSERT_EQ(buffer.size(), advance + str.size() + 1);
	std::array<char, str.size() + 1> out{};
	buffer.read(out.data(), advance); // skip the 'empty' data
	buffer.read(out.data(), str.size() + 1);
	ASSERT_STREQ(str.data(), out.data());
}

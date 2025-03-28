//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#define HEXI_BUFFER_DEBUG
#include <hexi/dynamic_buffer.h>
#include <hexi/buffer_sequence.h>
#undef HEXI_BUFFER_DEBUG
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <cstring>

using namespace std::literals;

TEST(dynamic_buffer, size) {
	hexi::dynamic_buffer<32> chain;
	ASSERT_EQ(0, chain.size()) << "Chain size is incorrect";

	chain.reserve(50);
	ASSERT_EQ(50, chain.size()) << "Chain size is incorrect";

	char buffer[20];
	chain.read(buffer, sizeof(buffer));
	ASSERT_EQ(30, chain.size()) << "Chain size is incorrect";

	chain.skip(10);
	ASSERT_EQ(20, chain.size()) << "Chain size is incorrect";

	chain.write(buffer, 20);
	ASSERT_EQ(40, chain.size()) << "Chain size is incorrect";

	chain.clear();
	ASSERT_EQ(0, chain.size()) << "Chain size is incorrect";
}

TEST(dynamic_buffer, read_write_consistency) {
	hexi::dynamic_buffer<32> chain;
	char text[] = "The quick brown fox jumps over the lazy dog";
	int num = 41521;

	chain.write(text, sizeof(text));
	chain.write(&num, sizeof(int));

	char text_out[sizeof(text)];
	int num_out;

	chain.read(text_out, sizeof(text));
	chain.read(&num_out, sizeof(int));

	ASSERT_EQ(num, num_out) << "Read produced incorrect result";
	ASSERT_STREQ(text, text_out) << "Read produced incorrect result";
	ASSERT_EQ(0, chain.size()) << "Chain should be empty";
}

TEST(dynamic_buffer, reserve_fetch_consistency) {
	hexi::dynamic_buffer<32> chain;
	char text[] = "The quick brown fox jumps over the lazy dog";
	const std::size_t text_len = sizeof(text);

	chain.reserve(text_len);
	ASSERT_EQ(text_len, chain.size()) << "Chain size is incorrect";

	auto buffers = chain.fetch_buffers(text_len);
	
	// store text in the retrieved buffers
	std::size_t offset = 0;

	for(auto& buffer : buffers) {
		std::memcpy(buffer->read_ptr(), text + offset, buffer->size());
		offset += buffer->size();

		if(offset > text_len || !offset) {
			break;
		}
	}

	// read text back out
	std::string output;

	output.resize_and_overwrite(text_len, [&](char* strbuf, std::size_t size) {
		chain.read(strbuf, size);
		return size;
	});

	ASSERT_EQ(0, chain.size()) << "Chain size is incorrect";
	ASSERT_STREQ(text, output.c_str()) << "Read produced incorrect result";
}

TEST(dynamic_buffer, skip) {
	hexi::dynamic_buffer<32> chain;
	int foo = 960;
	int bar = 296;

	chain.write(&foo, sizeof(int));
	chain.write(&bar, sizeof(int));
	chain.skip(sizeof(int));
	chain.read(&foo, sizeof(int));

	ASSERT_EQ(0, chain.size()) << "Chain size is incorrect";
	ASSERT_EQ(bar, foo) << "Skip produced incorrect result";
}

TEST(dynamic_buffer, clear) {
	hexi::dynamic_buffer<32> chain;
	const int iterations = 100;

	for(int i = 0; i < iterations; ++i) {
		chain.write(&i, sizeof(int));
	}

	ASSERT_EQ(sizeof(int) * iterations, chain.size()) << "Chain size is incorrect";
	chain.clear();
	ASSERT_EQ(0, chain.size()) << "Chain size is incorrect";
	ASSERT_TRUE(chain.empty());
	ASSERT_EQ(0, chain.block_count());
}

TEST(dynamic_buffer, attach_tail) {
	hexi::dynamic_buffer<32> chain;
	auto buffer = chain.get_allocator().allocate();

	std::string text("This is a string that is almost certainly longer than 32 bytes");
	std::size_t written = buffer->write(text.c_str(), text.length());

	ASSERT_EQ(0, chain.size()) << "Chain size is incorrect";
	chain.push_back(buffer);
	chain.skip(32); // skip first block
	chain.advance_write(written);
	ASSERT_EQ(written, chain.size()) << "Chain size is incorrect";

	std::string output;

	output.resize_and_overwrite(written, [&](char* strbuf, std::size_t size) {
		chain.read(strbuf, size);
		return size;
	});

	ASSERT_EQ(0, chain.size()) << "Chain size is incorrect";
	ASSERT_EQ(0, std::memcmp(text.data(), output.data(), written)) << "Output is incorrect";
}

TEST(dynamic_buffer, pop_front_push_back) {
	hexi::dynamic_buffer<32> chain;
	auto buffer = chain.get_allocator().allocate();

	std::string text("This is a string that is almost certainly longer than 32 bytes");
	std::size_t written = buffer->write(text.c_str(), text.length());

	ASSERT_EQ(0, chain.size()) << "Chain size is incorrect";
	chain.push_back(buffer);
	ASSERT_EQ(written, chain.size()) << "Chain size is incorrect";
	chain.pop_front();
	ASSERT_EQ(0, chain.size()) << "Chain size is incorrect";
}

TEST(dynamic_buffer, retrieve_tail) {
	hexi::dynamic_buffer<32> chain;
	std::string text("This string is < 32 bytes"); // could this fail on exotic platforms?
	chain.write(text.data(), text.length());

	auto tail = chain.back();
	ASSERT_TRUE(tail);
	ASSERT_EQ(0, std::memcmp(text.data(), tail->storage.data(), text.length())) << "Tail data is incorrect";
}

TEST(dynamic_buffer, copy) {
	hexi::dynamic_buffer<32> chain;
	int output, foo = 54543;
	chain.write(&foo, sizeof(int));
	ASSERT_EQ(sizeof(int), chain.size());
	chain.copy(&output, sizeof(int));
	ASSERT_EQ(sizeof(int), chain.size()) << "Chain size is incorrect";
	ASSERT_EQ(foo, output) << "Copy output is incorrect";
}

TEST(dynamic_buffer, copy_chain) {
	hexi::dynamic_buffer<sizeof(int)> chain, chain2;
	int foo = 5491;
	int output;

	chain.write(&foo, sizeof(int));
	chain.write(&foo, sizeof(int));
	ASSERT_EQ(sizeof(int) * 2, chain.size()) << "Chain size is incorrect";
	ASSERT_EQ(0, chain2.size()) << "Chain size is incorrect";

	chain2 = chain;
	ASSERT_EQ(sizeof(int) * 2, chain.size()) << "Chain size is incorrect";
	ASSERT_EQ(sizeof(int) * 2, chain2.size()) << "Chain size is incorrect";

	chain.read(&output, sizeof(int));
	ASSERT_EQ(sizeof(int), chain.size()) << "Chain size is incorrect";
	ASSERT_EQ(sizeof(int) * 2, chain2.size()) << "Chain size is incorrect";

	chain.clear();
	ASSERT_EQ(0, chain.size()) << "Chain size is incorrect";
	ASSERT_EQ(sizeof(int) * 2, chain2.size()) << "Chain size is incorrect";

	chain2.read(&output, sizeof(int));
	ASSERT_EQ(foo, output) << "Chain output is incorrect";
}

TEST(dynamic_buffer, move_chain) {
	hexi::dynamic_buffer<32> chain, chain2;
	int foo = 23113;

	chain.write(&foo, sizeof(int));
	ASSERT_EQ(sizeof(int), chain.size()) << "Chain size is incorrect";
	ASSERT_EQ(0, chain2.size()) << "Chain size is incorrect";

	chain2 = std::move(chain);
	ASSERT_EQ(sizeof(int), chain2.size()) << "Chain size is incorrect";

	int output;
	chain2.read(&output, sizeof(int));
	ASSERT_EQ(foo, output) << "Chain output is incorrect";
}

TEST(dynamic_buffer, write_seek) {
	hexi::dynamic_buffer<1> chain; // ensure the data is split over multiple buffer nodes
	const std::array<std::uint8_t, 6> data {0x00, 0x01, 0x00, 0x00, 0x04, 0x05};
	const std::array<std::uint8_t, 2> seek_data {0x02, 0x03};
	const std::array<std::uint8_t, 4> expected_data {0x00, 0x01, 0x02, 0x03};

	chain.write(data.data(), data.size());
	chain.write_seek(hexi::buffer_seek::sk_backward, 4);
	chain.write(seek_data.data(), seek_data.size());

	// make sure the chain is four bytes (6 written, (-)4 rewound, (+)2 rewritten = 4)
	ASSERT_EQ(chain.size(), 4) << "Chain size is incorrect";

	std::vector<std::uint8_t> out(chain.size());
	chain.copy(out.data(), out.size());

	ASSERT_EQ(0, std::memcmp(out.data(), expected_data.data(), expected_data.size()))
		<< "Buffer contains incorrect data pattern";

	// put the write cursor back to its original position and write more data
	chain.write_seek(hexi::buffer_seek::sk_forward, 2);

	// should be six bytes in there
	ASSERT_EQ(chain.size(), data.size()) << "Chain size is incorrect";

	const std::array<std::uint8_t, 8> final_data {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
	const std::array<std::uint8_t, 2> new_data {0x06, 0x07};
	chain.write(new_data.data(), new_data.size());

	// should be eight bytes in there
	ASSERT_EQ(chain.size(), final_data.size()) << "Chain size is incorrect";

	// check the pattern/sequence/whatever
	out.resize(chain.size());
	chain.read(out.data(), out.size());

	ASSERT_EQ(0, std::memcmp(out.data(), final_data.data(), final_data.size()))
		<< "Buffer contains incorrect data pattern";
}

#if defined HEXI_WITH_ASIO || defined HEXI_WITH_BOOST_ASIO
TEST(dynamic_buffer, read_iterator) {
	hexi::dynamic_buffer<16> chain; // ensure the string is split over multiple buffers
	hexi::buffer_sequence sequence(chain);
	std::string skip("Skipping");
	std::string input("The quick brown fox jumps over the lazy dog");
	std::string output;

	chain.write(skip.data(), skip.size());
	chain.write(input.data(), input.size());
	chain.skip(skip.size()); // ensure skipped data isn't read back out
	
	for(auto i = sequence.begin(), j = sequence.end(); i != j; ++i) {
		auto buffer = i.get_buffer();
		std::copy(buffer.data(), buffer.data()  + buffer.size(), std::back_inserter(output));
	}

	ASSERT_EQ(input, output) << "Read iterator produced incorrect result";
}

TEST(dynamic_buffer, asio_iterator_regression_test) {
	hexi::dynamic_buffer<1> chain;
	hexi::buffer_sequence sequence(chain);

	// 119 bytes (size of 1.12.1 LoginChallenge packet)
	std::string input("Lorem ipsum dolor sit amet, consectetur adipiscing elit."
	                  " Etiam sagittis pulvinar massa nec pellentesque. Integer metus.");

	chain.write(input.data(), input.length());
	ASSERT_EQ(119, chain.size()) << "Chain size was incorrect";

	auto it = sequence.begin();
	std::size_t bytes_sent = 0;

	// do first read
	for(std::size_t i = 0; it != sequence.end() && i != 64; ++i, ++it) {
		bytes_sent += it.get_buffer().size();
	}

	chain.skip(bytes_sent);
	ASSERT_EQ(64, bytes_sent) << "First read length was incorrect";
	ASSERT_EQ(55, chain.size()) << "Chain size was incorrect";

	auto it_s = sequence.begin();
	bytes_sent = 0;

	// do second read
	for(std::size_t i = 0; it_s != sequence.end() && i != 64; ++i, ++it_s) {
		bytes_sent += it_s.get_buffer().size();
	}

	chain.skip(bytes_sent);
	ASSERT_EQ(55, bytes_sent) << "Second read length was incorrect";
	ASSERT_EQ(0, chain.size()) << "Chain size was incorrect";

	//chain.clear();
	input = "xy"; // two bytes - size of LoginProof if failed login
	chain.write(input.data(), input.length());
	ASSERT_EQ(2, chain.size()) << "Chain size was incorrect";

	auto it_t = sequence.begin();
	bytes_sent = 0;

	// do third read
	for(std::size_t i = 0; it_t != sequence.end() && i != 64; ++i, ++it_t) {
		bytes_sent += it_t.get_buffer().size();
	}

	chain.skip(bytes_sent);
	ASSERT_EQ(2, bytes_sent) << "Regression found - read length was incorrect";
	ASSERT_EQ(0, chain.size()) << "Chain size was incorrect";
}

#endif 

TEST(dynamic_buffer, empty) {
	hexi::dynamic_buffer<32> chain;
	ASSERT_TRUE(chain.empty());
	const auto value = 0;
	chain.write(&value, sizeof(value));
	ASSERT_FALSE(chain.empty());
}

TEST(dynamic_buffer, block_size) {
	hexi::dynamic_buffer<32> chain;
	ASSERT_EQ(chain.block_size(), 32);
	hexi::dynamic_buffer<64> chain2;
	ASSERT_EQ(chain2.block_size(), 64);
}

TEST(dynamic_buffer, block_count) {
	hexi::dynamic_buffer<1> chain;
	const std::uint8_t value = 0;
	chain.write(&value, sizeof(value));
	ASSERT_EQ(chain.block_count(), 1);
	chain.write(&value, sizeof(value));
	ASSERT_EQ(chain.block_count(), 2);
	chain.write(&value, sizeof(value));
	chain.write(&value, sizeof(value));
	ASSERT_EQ(chain.block_count(), 4);
	chain.pop_front();
	ASSERT_EQ(chain.block_count(), 3);
}

TEST(dynamic_buffer, find_first_of) {
	hexi::dynamic_buffer<64> buffer;
	const auto str = "The quick brown fox jumped over the lazy dog"sv;
	buffer.write(str.data(), str.size());
	auto pos = buffer.find_first_of(std::byte('\0'));
	ASSERT_EQ(pos, buffer.npos); // direct string write is not terminated
	pos = buffer.find_first_of(std::byte('g'));
	ASSERT_EQ(pos, 43);
	pos = buffer.find_first_of(std::byte('T'));
	ASSERT_EQ(pos, 0);
	pos = buffer.find_first_of(std::byte('t'));
	ASSERT_EQ(pos, 32);
}
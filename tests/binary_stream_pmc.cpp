//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#include <hexi/pmc/binary_stream.h>
#include <hexi/pmc/buffer_adaptor.h>
#include <hexi/dynamic_buffer.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <numeric>
#include <random>
#include <cstdint>
#include <cstdlib>

TEST(binary_stream_pmc, message_read_limit) {
	std::array<std::uint8_t, 14> ping {
		0x00, 0x0C, 0xDC, 0x01, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0xF4, 0x01, 0x00, 0x00
	};

	// write the ping packet data twice to the buffer
	hexi::dynamic_buffer<32> buffer;
	buffer.write(ping.data(), ping.size());
	buffer.write(ping.data(), ping.size());

	// read one packet back out (reuse the ping array)
	hexi::pmc::binary_stream stream(buffer, ping.size());
	ASSERT_EQ(stream.read_limit(), ping.size());
	ASSERT_NO_THROW(stream.get(ping.data(), ping.size()))
		<< "Failed to read packet back from stream";

	// attempt to read past the stream message bound
	ASSERT_THROW(stream.get(ping.data(), ping.size()), hexi::stream_read_limit)
		<< "Message boundary was not respected";
	ASSERT_EQ(stream.state(), hexi::stream_state::read_limit_err)
		<< "Unexpected stream state";
}

TEST(binary_stream_pmc, buffer_limit) {
	std::array<std::uint8_t, 14> ping {
		0x00, 0x0C, 0xDC, 0x01, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0xF4, 0x01, 0x00, 0x00
	};

	// write the ping packet data to the buffer
	hexi::dynamic_buffer<32> buffer;
	buffer.write(ping.data(), ping.size());

	// read all data back out
	hexi::pmc::binary_stream stream(buffer);
	ASSERT_NO_THROW(stream.get(ping.data(), ping.size()))
		<< "Failed to read packet back from stream";

	// attempt to read past the buffer bound
	ASSERT_THROW(stream.get(ping.data(), ping.size()), hexi::buffer_underrun)
		<< "Message boundary was not respected";
	ASSERT_EQ(stream.state(), hexi::stream_state::buff_limit_err)
		<< "Unexpected stream state";
}

TEST(binary_stream_pmc, read_write_ints) {
	hexi::dynamic_buffer<32> buffer;
	hexi::pmc::binary_stream stream(buffer);

	const std::uint16_t in { 100 };
	stream << in;

	ASSERT_EQ(stream.size(), sizeof(in));
	ASSERT_EQ(stream.size(), buffer.size());

	std::uint16_t out;
	stream >> out;

	ASSERT_EQ(in, out);
	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(buffer.empty());
	ASSERT_EQ(stream.state(), hexi::stream_state::ok)
		<< "Unexpected stream state";
}

TEST(binary_stream_pmc, read_write_std_string) {
	hexi::dynamic_buffer<32> buffer;
	hexi::pmc::binary_stream stream(buffer);
	const std::string in { "The quick brown fox jumped over the lazy dog" };
	stream << hexi::null_terminated(in);

	// +1 to account for the terminator that's written
	ASSERT_EQ(stream.size(), in.size() + 1);

	std::string out;
	stream >> hexi::null_terminated(out);

	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(in, out);
	ASSERT_EQ(stream.state(), hexi::stream_state::ok)
		<< "Unexpected stream state";
}

TEST(binary_stream_pmc, read_write_c_string) {
	hexi::dynamic_buffer<32> buffer;
	hexi::pmc::binary_stream stream(buffer);
	const char* in { "The quick brown fox jumped over the lazy dog" };
	stream << in;

	ASSERT_EQ(stream.size(), strlen(in) + 1);

	std::string out;
	stream >> hexi::null_terminated(out);

	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(0, strcmp(in, out.c_str()));
	ASSERT_EQ(stream.state(), hexi::stream_state::ok)
		<< "Unexpected stream state";
}

TEST(binary_stream_pmc, read_write_vector) {
	hexi::dynamic_buffer<32> buffer;
	hexi::pmc::binary_stream stream(buffer);

	const auto time = std::chrono::system_clock::now().time_since_epoch();
	const unsigned int seed = static_cast<unsigned int>(time.count());
	std::srand(seed);

	std::vector<int> in(std::rand() % 200);
	std::ranges::iota(in, std::rand() % 100);
	std::ranges::shuffle(in, std::default_random_engine(seed));

	stream.put(in.begin(), in.end());

	ASSERT_EQ(stream.size(), in.size() * sizeof(int));

	// read the integers back one by one, making sure they
	// match the expected value
	for(auto value : in) {
		int output = -1;
		stream >> output;
		ASSERT_EQ(output, value);
	}

	stream.put(in.begin(), in.end());
	std::vector<int> out(in.size());

	// read the integers to an output buffer and compare both
	stream.get(out.begin(), out.end());
	ASSERT_EQ(in, out);
	ASSERT_EQ(stream.state(), hexi::stream_state::ok)
		<< "Unexpected stream state";
}

TEST(binary_stream_pmc, clear) {
	hexi::dynamic_buffer<32> buffer;
	hexi::pmc::binary_stream stream(buffer);
	stream << 0xBADF00D;

	ASSERT_TRUE(!stream.empty());
	ASSERT_TRUE(!buffer.empty());

	stream.skip(stream.size());

	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(buffer.empty());
}

TEST(binary_stream_pmc, skip) {
	hexi::dynamic_buffer<32> buffer;
	hexi::pmc::binary_stream stream(buffer);

	const std::uint64_t in {0xBADF00D};
	stream << in << in;
	stream.skip(sizeof(in));

	ASSERT_EQ(stream.size(), sizeof(in));
	ASSERT_EQ(stream.size(), buffer.size());

	std::uint64_t out;
	stream >> out;

	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(in, out);
}

TEST(binary_stream_pmc, can_write_seek) {
	hexi::dynamic_buffer<32> buffer;
	hexi::pmc::binary_stream stream(buffer);
	ASSERT_EQ(buffer.can_write_seek(), stream.can_write_seek());
}

TEST(binary_stream_pmc, get_put) {
	hexi::dynamic_buffer<32> buffer;
	hexi::pmc::binary_stream stream(buffer);
	std::vector<std::uint8_t> in { 1, 2, 3, 4, 5 };
	std::vector<std::uint8_t> out(in.size());

	stream.put(in.data(), in.size());
	stream.get(out.data(), out.size());

	ASSERT_EQ(stream.total_read(), out.size());
	ASSERT_EQ(stream.total_write(), in.size());
	ASSERT_EQ(in, out);
}

TEST(binary_stream_pmc, fill) {
	std::vector<std::uint8_t> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	hexi::pmc::binary_stream stream(adaptor);
	stream.fill<30>(128);
	ASSERT_EQ(buffer.size(), 30);
	ASSERT_EQ(stream.total_write(), 30);
	stream.fill<2>(128);
	ASSERT_EQ(buffer.size(), 32);
	ASSERT_EQ(stream.total_write(), 32);
	auto it = std::ranges::find_if(buffer,  [](auto i) { return i != 128; });
	ASSERT_EQ(it, buffer.end());
}

TEST(binary_stream_pmc, array) {
	std::vector<char> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	hexi::pmc::binary_stream stream(adaptor);
	const int arr[] = { 1, 2, 3 };
	stream << arr;
	int val = 0;
	stream >> val;
	ASSERT_EQ(val, 1);
	stream >> val;
	ASSERT_EQ(val, 2);
	stream >> val;
	ASSERT_EQ(val, 3);
}

TEST(binary_stream_pmc, put_integral_literals) {
	hexi::dynamic_buffer<64> buffer;
	hexi::pmc::binary_stream stream(buffer);
	stream.put<std::uint64_t>(std::numeric_limits<std::uint64_t>::max());
	stream.put<std::uint32_t>(std::numeric_limits<std::uint32_t>::max());
	stream.put<std::uint16_t>(std::numeric_limits<std::uint16_t>::max());
	stream.put<std::uint8_t>(std::numeric_limits<std::uint8_t>::max());
	stream.put<std::int64_t>(std::numeric_limits<std::int64_t>::max());
	stream.put<std::int32_t>(std::numeric_limits<std::int32_t>::max());
	stream.put<std::int16_t>(std::numeric_limits<std::int16_t>::max());
	stream.put<std::int8_t>(std::numeric_limits<std::int8_t>::max());
	stream.put(1.5f);
	stream.put(3.0);
	std::uint64_t resultu64 = 0;
	std::uint32_t resultu32 = 0;
	std::uint16_t resultu16 = 0;
	std::uint8_t resultu8 = 0;
	std::int64_t result64 = 0;
	std::int32_t result32 = 0;
	std::int16_t result16 = 0;
	std::int8_t result8 = 0;
	float resultf = 0.0f;
	double resultd = 0.0;
	stream >> resultu64 >> resultu32 >> resultu16 >> resultu8;
	stream >> result64 >> result32 >> result16 >> result8;
	stream >> resultf >> resultd;
	ASSERT_FLOAT_EQ(1.5f, resultf);
	ASSERT_DOUBLE_EQ(3.0, resultd);
	ASSERT_EQ(resultu8, std::numeric_limits<std::uint8_t>::max());
	ASSERT_EQ(resultu16, std::numeric_limits<std::uint16_t>::max());
	ASSERT_EQ(resultu32, std::numeric_limits<std::uint32_t>::max());
	ASSERT_EQ(resultu64, std::numeric_limits<std::uint64_t>::max());
	ASSERT_EQ(result8, std::numeric_limits<std::int8_t>::max());
	ASSERT_EQ(result16, std::numeric_limits<std::int16_t>::max());
	ASSERT_EQ(result32, std::numeric_limits<std::int32_t>::max());
	ASSERT_EQ(result64, std::numeric_limits<std::int64_t>::max());
	ASSERT_TRUE(stream);
}

TEST(binary_stream_pmc, string_view_write) {
	std::string buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	hexi::pmc::binary_stream stream(adaptor);
	std::string_view view { "There's coffee in that nebula" };
	stream << view;
	std::string res;
	stream >> res;
	ASSERT_EQ(view, res);
}

TEST(binary_stream_pmc, set_error_state) {
	std::string buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	hexi::pmc::binary_stream stream(adaptor);
	ASSERT_TRUE(stream);
	ASSERT_TRUE(stream.good());
	ASSERT_TRUE(stream.state() == hexi::stream_state::ok);
	stream.set_error_state();
	ASSERT_FALSE(stream);
	ASSERT_FALSE(stream.good());
	ASSERT_TRUE(stream.state() == hexi::stream_state::user_defined_err);
}

TEST(binary_streamPMR, StringAdaptor_PrefixedVarint_Long) {
	std::vector<char> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	hexi::pmc::binary_stream stream(adaptor);

	std::string input;

	// encode varint requiring three bytes
	input.resize_and_overwrite(80'000, [&](char* buffer, const std::size_t size) {
		for(std::size_t i = 0; i < size; ++i) {
			buffer[i] = (rand() % 127) + 32; // ASCII a-z
		}

		return size;
	});
	
	stream << hexi::prefixed_varint(input);
	std::string output;
	stream >> hexi::prefixed_varint(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(stream);
}

TEST(binary_streamPMR, StringAdaptor_PrefixedVarint_Medium) {
	std::vector<char> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	hexi::pmc::binary_stream stream(adaptor);

	std::string input;

	// encode varint requiring two bytes
	input.resize_and_overwrite(5'000, [&](char* buffer, const std::size_t size) {
		for(std::size_t i = 0; i < size; ++i) {
			buffer[i] = (rand() % 127) + 32; // ASCII a-z
		}

		return size;
	});
	
	stream << hexi::prefixed_varint(input);
	std::string output;
	stream >> hexi::prefixed_varint(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(stream);
}

TEST(binary_streamPMR, StringAdaptor_PrefixedVarint_Short) {
	std::vector<char> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	hexi::pmc::binary_stream stream(adaptor);

	std::string input;

	// encode varint requiring one byte
	input.resize_and_overwrite(127, [&](char* buffer, const std::size_t size) {
		for(std::size_t i = 0; i < size; ++i) {
			buffer[i] = (rand() % 127) + 32; // ASCII a-z
		}

		return size;
	});
	
	stream << hexi::prefixed_varint(input);
	std::string output;
	stream >> hexi::prefixed_varint(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(stream);
}

TEST(binary_streamPMR, StringAdaptor_Prefixed) {
	std::vector<char> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	hexi::pmc::binary_stream stream(adaptor);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << hexi::prefixed(input);
	std::string output;
	stream >> hexi::prefixed(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(binary_streamPMR, StringAdaptor_Default) {
	std::vector<char> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	hexi::pmc::binary_stream stream(adaptor);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << input;
	std::string output;
	stream >> output;
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(binary_streamPMR, StringAdaptor_Raw) {
	std::vector<char> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	hexi::pmc::binary_stream stream(adaptor);
	const auto input = std::format("String with {} embedded null", '\0');
	stream << hexi::raw(input);
	ASSERT_EQ(input.size(), buffer.size());
	std::string output;
	stream >> hexi::null_terminated(output);
	ASSERT_EQ(output, "String with ");
	ASSERT_FALSE(stream.empty());
}

TEST(binary_streamPMR, StringAdaptor_NullTerminated) {
	std::vector<char> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	hexi::pmc::binary_stream stream(adaptor);
	const std::string input { "We're just normal strings. Innocent strings."};
	stream << hexi::null_terminated(input);
	std::string output;
	stream >> hexi::null_terminated(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(binary_streamPMR, StringviewAdaptor_Prefixed) {
	std::vector<char> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	hexi::pmc::binary_stream stream(adaptor);
	std::string_view input { "The quick brown fox jumped over the lazy dog" };
	stream << hexi::prefixed(input);
	std::string output;
	stream >> hexi::prefixed(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(binary_streamPMR, StringviewAdaptor_Default) {
	std::vector<char> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	hexi::pmc::binary_stream stream(adaptor);
	std::string_view input { "The quick brown fox jumped over the lazy dog" };
	stream << input;
	std::string output;
	stream >> output;
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(binary_streamPMR, StringviewAdaptor_Raw) {
	std::vector<char> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	hexi::pmc::binary_stream stream(adaptor);
	const auto str = std::format("String with {} embedded null", '\0');
	std::string_view input { str };
	stream << hexi::raw(input);
	ASSERT_EQ(input.size(), buffer.size());
	std::string output;
	stream >> hexi::null_terminated(output);
	ASSERT_EQ(output, "String with ");
	ASSERT_FALSE(stream.empty());
}

TEST(binary_streamPMR, StringviewAdaptor_NullTerminated) {
	std::vector<char> buffer;
	hexi::pmc::buffer_adaptor adaptor(buffer);
	hexi::pmc::binary_stream stream(adaptor);
	std::string_view input { "We're just normal strings. Innocent strings." };
	stream << hexi::null_terminated(input);
	ASSERT_EQ(stream.size(), input.size() + 1); // account for the null terminator
	std::string output;
	stream >> hexi::null_terminated(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

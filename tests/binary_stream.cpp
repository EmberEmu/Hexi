//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#include "final_action.h"
#include "helpers.h"
#include <hexi/dynamic_buffer.h>
#include <hexi/endian.h>
#include <hexi/file_buffer.h>
#include <hexi/static_buffer.h>
#include <hexi/binary_stream.h>
#include <hexi/buffer_adaptor.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <numeric>
#include <random>
#include <cstdint>
#include <cstdlib>
#include <cstring>

TEST(binary_stream, message_read_limit) {
	std::array<std::uint8_t, 14> ping {
		0x00, 0x0C, 0xDC, 0x01, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0xF4, 0x01, 0x00, 0x00
	};

	// write the ping packet data twice to the buffer
	hexi::dynamic_buffer<32> buffer;
	buffer.write(ping.data(), ping.size());
	buffer.write(ping.data(), ping.size());

	// read one packet back out (reuse the ping array)
	hexi::binary_stream stream(buffer, ping.size());
	ASSERT_EQ(stream.read_limit(), ping.size());
	ASSERT_NO_THROW(stream.get(ping.data(), ping.size()))
		<< "Failed to read packet back from stream";

	// attempt to read past the stream message bound
	ASSERT_THROW(stream.get(ping.data(), ping.size()), hexi::stream_read_limit)
		<< "Message boundary was not respected";
	ASSERT_EQ(stream.state(), hexi::stream_state::read_limit_err)
		<< "Unexpected stream state";
}

TEST(binary_stream, buffer_limit) {
	std::array<std::uint8_t, 14> ping {
		0x00, 0x0C, 0xDC, 0x01, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0xF4, 0x01, 0x00, 0x00
	};

	// write the ping packet data to the buffer
	hexi::dynamic_buffer<32> buffer;
	buffer.write(ping.data(), ping.size());

	// read all data back out
	hexi::binary_stream stream(buffer);
	ASSERT_NO_THROW(stream.get(ping.data(), ping.size()))
		<< "Failed to read packet back from stream";

	// attempt to read past the buffer bound
	ASSERT_THROW(stream.get(ping.data(), ping.size()), hexi::buffer_underrun)
		<< "Message boundary was not respected";
	ASSERT_EQ(stream.state(), hexi::stream_state::buff_limit_err)
		<< "Unexpected stream state";
}

TEST(binary_stream, read_write_ints) {
	hexi::dynamic_buffer<32> buffer;
	hexi::binary_stream stream(buffer);

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

TEST(binary_stream, read_write_std_string) {
	hexi::dynamic_buffer<32> buffer;
	hexi::binary_stream stream(buffer);
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

TEST(binary_stream, read_write_c_string) {
	hexi::dynamic_buffer<32> buffer;
	hexi::binary_stream stream(buffer);
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

TEST(binary_stream, read_write_vector) {
	hexi::dynamic_buffer<32> buffer;
	hexi::binary_stream stream(buffer);

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
	for(auto it = in.begin(); it != in.end(); ++it) {
		int output;
		stream >> output;
		ASSERT_EQ(output, *it);
	}

	stream.put(in.begin(), in.end());
	std::vector<int> out(in.size());

	// read the integers to an output buffer and compare both
	stream.get(out.begin(), out.end());
	ASSERT_EQ(in, out);
	ASSERT_EQ(stream.state(), hexi::stream_state::ok)
		<< "Unexpected stream state";
}

TEST(binary_stream, clear) {
	hexi::dynamic_buffer<32> buffer;
	hexi::binary_stream stream(buffer);
	stream << 0xBADF00D;

	ASSERT_TRUE(!stream.empty());
	ASSERT_TRUE(!buffer.empty());

	stream.skip(stream.size());

	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(buffer.empty());
}

TEST(binary_stream, skip) {
	hexi::dynamic_buffer<32> buffer;
	hexi::binary_stream stream(buffer);

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

TEST(binary_stream, can_write_seek) {
	hexi::dynamic_buffer<32> buffer;
	hexi::binary_stream stream(buffer);
	ASSERT_EQ(buffer.can_write_seek(), stream.can_write_seek());
}

TEST(binary_stream, get_put) {
	hexi::dynamic_buffer<32> buffer;
	hexi::binary_stream stream(buffer);
	std::vector<std::uint8_t> in { 1, 2, 3, 4, 5 };
	std::vector<std::uint8_t> out(in.size());

	stream.put(in.data(), in.size());
	stream.get(out.data(), out.size());

	ASSERT_EQ(stream.total_read(), out.size());
	ASSERT_EQ(stream.total_write(), in.size());
	ASSERT_EQ(in, out);
}

TEST(binary_stream, fill) {
	std::vector<std::uint8_t> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);
	stream.fill<30>(128);
	ASSERT_EQ(buffer.size(), 30);
	ASSERT_EQ(stream.total_write(), 30);
	stream.fill<2>(128);
	ASSERT_EQ(buffer.size(), 32);
	ASSERT_EQ(stream.total_write(), 32);
	auto it = std::ranges::find_if(buffer,  [](auto i) { return i != 128; });
	ASSERT_EQ(it, buffer.end());
}

TEST(binary_stream, no_copy_string_read) {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	const std::uint32_t trailing { 0x0DDBA11 };
	stream << hexi::null_terminated(input) << trailing;

	// check this stream uses a contiguous buffer
	const auto contig = std::is_same_v<decltype(stream)::contiguous_type, hexi::is_contiguous>;
	ASSERT_TRUE(contig);

	// find the end of the string within the buffer
	const auto stream_buf = stream.buffer();
	const auto pos = stream_buf->find_first_of('\0');
	ASSERT_NE(pos, adaptor.npos);

	// create a view into the buffer & skip ahead so the next read continues as normal
	std::string_view output(stream_buf->read_ptr(), pos);
	ASSERT_EQ(input, output);

	// ensure we can still read subsequent data as normal
	stream.skip(pos + 1); // +1 to skip terminator
	std::uint32_t trailing_output = 0;
	stream >> trailing_output;
	ASSERT_EQ(trailing, trailing_output);
}

TEST(binary_stream, string_view_read) {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	const std::uint32_t trailing { 0x0DDBA11 };
	stream << hexi::null_terminated(input) << trailing;

	auto view = stream.view();
	ASSERT_EQ(input, view);

	// ensure we can still read subsequent data as normal
	std::uint32_t trailing_output = 0;
	stream >> trailing_output;
	ASSERT_EQ(trailing, trailing_output);
}

TEST(binary_stream, partial_string_view_read) {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << hexi::null_terminated(input);

	auto span = stream.span(20);
	std::string_view view(span);
	ASSERT_EQ("The quick brown fox ", view);

	// read the rest of the string
	view = stream.view();
	ASSERT_EQ("jumped over the lazy dog", view);
	ASSERT_TRUE(stream.empty());
}

TEST(binary_stream, string_view_stream) {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	const std::uint32_t trailing { 0xDEFECA7E };
	stream << hexi::null_terminated(input) << trailing;

	std::string_view output;
	stream >> hexi::null_terminated(output);
	ASSERT_EQ(input, output);

	// ensure we can still read subsequent data as normal
	std::uint32_t trailing_output = 0;
	stream >> trailing_output;
	ASSERT_EQ(trailing, trailing_output);
	
	// make a sly modification to the buffer and check the string_view matches
	ASSERT_FALSE(buffer.empty());
	buffer[0] = 'A';
	ASSERT_EQ(buffer[0], output[0]);
	ASSERT_NE(input, output);
}

TEST(binary_stream, array) {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);
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

TEST(binary_stream, span) {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);
	const int arr[] = { 4, 9, 2, 1 }; // chosen by fair dice roll
	stream << arr;
	auto span = stream.span<int>(4);
	ASSERT_TRUE(stream.empty());
	// not checking directly against the array in case it somehow gets clobbered,
	// which would mean the span would get clobbered and succeed where it shouldn't
	ASSERT_EQ(span[0], 4);
	ASSERT_EQ(span[1], 9);
	ASSERT_EQ(span[2], 2);
	ASSERT_EQ(span[3], 1);
}

TEST(binary_stream, static_buffer_write) {
	hexi::static_buffer<char, 4> buffer;
	hexi::binary_stream stream(buffer);
	stream << 'a' << 'b' << 'c' << 'd';
	ASSERT_EQ(buffer[0], 'a');
	ASSERT_EQ(buffer[1], 'b');
	ASSERT_EQ(buffer[2], 'c');
	ASSERT_EQ(buffer[3], 'd');
	ASSERT_TRUE(stream);
}

TEST(binary_stream, static_buffer_direct_write) {
	hexi::static_buffer<char, 4> buffer;
	hexi::binary_stream stream(buffer);
	std::uint32_t input = 0xBEE5BEE5;
	std::uint32_t output = 0;
	buffer.write(&input, sizeof(input));
	stream >> output;
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream);
}

TEST(binary_stream, static_buffer_overflow) {
	hexi::static_buffer<char, 4> buffer;
	hexi::binary_stream stream(buffer);
	ASSERT_THROW(stream << std::uint64_t(1), hexi::buffer_overflow);
	ASSERT_FALSE(stream);
}

TEST(binary_stream, static_buffer_read) {
	hexi::static_buffer<char, 4> buffer;
	const std::uint32_t input = 0x11223344;
	buffer.write(&input, sizeof(input));
	hexi::binary_stream stream(buffer);
	std::uint32_t output = 0;
	stream >> output;
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream);
}

TEST(binary_stream, static_buffer_underrun) {
	hexi::static_buffer<char, 4> buffer;
	hexi::binary_stream stream(buffer);
	std::uint32_t input = 0xBEEFBEEF;
	std::uint32_t output = 0;
	stream << input;
	stream >> output;
	ASSERT_THROW(stream >> output, hexi::buffer_underrun);
	ASSERT_FALSE(stream);
	ASSERT_EQ(input, output);
}

TEST(binary_stream, static_buffer_underrun_noexcept) {
	hexi::static_buffer<char, 4> buffer;
	hexi::binary_stream stream(buffer, hexi::no_throw);
	std::uint32_t output = 0;
	stream << output;
	stream >> output;
	ASSERT_NO_THROW(stream >> output);
	ASSERT_FALSE(stream);
	ASSERT_EQ(output, 0);
}

TEST(binary_stream, put_integral_literals) {
	hexi::static_buffer<char, 64> buffer;
	hexi::binary_stream stream(buffer);
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

TEST(binary_stream, file_buffer_read) {
	std::filesystem::path path("data/filebuffer");
	ASSERT_TRUE(std::filesystem::exists(path));

	hexi::file_buffer buffer(path);
	ASSERT_TRUE(buffer) << "File open failed";

	hexi::binary_stream stream(buffer);
	std::uint8_t w = 0;
	std::uint16_t x = 0;
	std::uint32_t y = 0;
	std::uint64_t z = 0;

	std::string_view strcmp { "The quick brown fox jumped over the lazy dog." };
	std::string str_out;

	stream >> w >> x >> y >> z >> hexi::null_terminated(str_out);
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

TEST(binary_stream, file_buffer_write) {
	std::filesystem::path path { "tmp_unittest_binary_stream_file_buffer_write" };

	final_action fa([path] {
		std::filesystem::remove(path);
	});

	// in case previous clean-up failed
	std::filesystem::remove(path);

	hexi::file_buffer buffer(path);
	ASSERT_TRUE(buffer) << "File open failed";

	hexi::binary_stream stream(buffer);

	std::uint8_t w = 47;
	std::uint16_t x = 49197;
	std::uint32_t y = 2173709693;
	std::uint64_t z = 1438110846748337907;
	std::string str { "The quick brown fox jumped over the lazy dog." };

	hexi::endian::native_to_little_inplace(x);
	hexi::endian::native_to_little_inplace(y);
	hexi::endian::native_to_little_inplace(z);

	stream << w << x << y << z << hexi::null_terminated(str);
	buffer.flush(); // ensure data is written before following read

	const auto reference = read_file("data/filebuffer");
	const auto created = read_file(path);
	ASSERT_TRUE(std::ranges::equal(reference, created));
}

TEST(binary_stream, set_error_state) {
	hexi::static_buffer<char, 64> buffer;
	hexi::binary_stream stream(buffer);
	ASSERT_TRUE(stream);
	ASSERT_TRUE(stream.good());
	ASSERT_TRUE(stream.state() == hexi::stream_state::ok);
	stream.set_error_state();
	ASSERT_FALSE(stream);
	ASSERT_FALSE(stream.good());
	ASSERT_TRUE(stream.state() == hexi::stream_state::user_defined_err);
}

TEST(binary_stream, string_adaptor_prefixed_varint_long) {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);

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

TEST(binary_stream, string_adaptor_prefixed_varint_medium) {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);

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

TEST(binary_stream, string_adaptor_prefixed_varint_short) {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);

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

TEST(binary_stream, string_adaptor_prefixed) {
	hexi::static_buffer<char, 128> buffer;
	hexi::binary_stream stream(buffer);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << hexi::prefixed(input);
	std::string output;
	stream >> hexi::prefixed(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(binary_stream, string_adaptor_default) {
	hexi::static_buffer<char, 128> buffer;
	hexi::binary_stream stream(buffer);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << input;
	std::string output;
	stream >> output;
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(binary_stream, string_adaptor_raw) {
	hexi::static_buffer<char, 128> buffer;
	hexi::binary_stream stream(buffer);
	const auto input = std::format("String with {} embedded null", '\0');
	stream << hexi::raw(input);
	ASSERT_EQ(input.size(), buffer.size());
	std::string output;
	stream >> hexi::null_terminated(output);
	ASSERT_EQ(output, "String with ");
	ASSERT_FALSE(stream.empty());
}

TEST(binary_stream, string_adaptor_null_terminated) {
	hexi::static_buffer<char, 128> buffer;
	hexi::binary_stream stream(buffer);
	const std::string input { "We're just normal strings. Innocent strings."};
	stream << hexi::null_terminated(input);
	std::string output;
	stream >> hexi::null_terminated(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(binary_stream, string_view_adaptor_prefixed_varint_long) {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);

	std::string input;

	// encode varint requiring three bytes
	input.resize_and_overwrite(80'000, [&](char* buffer, const std::size_t size) {
		for(std::size_t i = 0; i < size; ++i) {
			buffer[i] = (rand() % 127) + 32; // ASCII a-z
		}

		return size;
	});

	stream << hexi::prefixed_varint(input);
	std::string_view output;
	stream >> hexi::prefixed_varint(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(stream);
}

TEST(binary_stream, string_view_adaptor_prefixed_varint_medium) {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);

	std::string input;

	// encode varint requiring two bytes
	input.resize_and_overwrite(5'000, [&](char* buffer, const std::size_t size) {
		for(std::size_t i = 0; i < size; ++i) {
			buffer[i] = (rand() % 127) + 32; // ASCII a-z
		}

		return size;
	});

	stream << hexi::prefixed_varint(input);
	std::string_view output;
	stream >> hexi::prefixed_varint(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(stream);
}

TEST(binary_stream, string_view_adaptor_prefixed_varint_short) {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);

	std::string input;

	// encode varint requiring one byte
	input.resize_and_overwrite(127, [&](char* buffer, const std::size_t size) {
		for(std::size_t i = 0; i < size; ++i) {
			buffer[i] = (rand() % 127) + 32; // ASCII a-z
		}

		return size;
	});

	stream << hexi::prefixed_varint(input);
	std::string_view output;
	stream >> hexi::prefixed_varint(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(stream);
}

TEST(binary_stream, string_view_adaptor_prefixed) {
	hexi::static_buffer<char, 128> buffer;
	hexi::binary_stream stream(buffer);
	std::string_view input { "The quick brown fox jumped over the lazy dog" };
	stream << hexi::prefixed(input);
	std::string_view output;
	stream >> hexi::prefixed(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(binary_stream, string_view_adaptor_default) {
	hexi::static_buffer<char, 128> buffer;
	hexi::binary_stream stream(buffer);
	std::string_view input { "The quick brown fox jumped over the lazy dog" };
	stream << input;
	std::string_view output;
	stream >> output;
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(binary_stream, string_view_adaptor_raw) {
	hexi::static_buffer<char, 128> buffer;
	hexi::binary_stream stream(buffer);
	const auto input = std::format("String with {} embedded null", '\0');
	stream << hexi::raw(input);
	ASSERT_EQ(input.size(), buffer.size());
	std::string_view output;
	stream >> hexi::null_terminated(output);
	ASSERT_EQ(output, "String with ");
	ASSERT_FALSE(stream.empty());
}

TEST(binary_stream, string_view_adaptor_null_terminated) {
	hexi::static_buffer<char, 128> buffer;
	hexi::binary_stream stream(buffer);
	std::string_view input { "We're just normal strings. Innocent strings."};
	stream << hexi::null_terminated(input);
	ASSERT_EQ(stream.size(), input.size() + 1); // +1, account for the terminator
	std::string_view output;
	stream >> hexi::null_terminated(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(binary_stream, std_array) {
	std::array<char, 128> buffer{};
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);
	std::string_view input { "We're just normal strings. Innocent strings."};
	
	// array is considered full by default as size == capacity
	ASSERT_THROW(stream << input, hexi::buffer_overflow);
	adaptor.clear();

	// try again now we've reset the state
	std::string_view output;
	stream << input;
	stream >> output;
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(binary_stream, total_write_consistency) {
	std::array<char, 256> buffer;
	hexi::buffer_adaptor adaptor(buffer, hexi::init_empty);
	hexi::binary_stream stream(adaptor);

	ASSERT_EQ(stream.total_write(), 0);
	stream << std::uint8_t(0);
	ASSERT_EQ(stream.total_write(), 1);
	stream << std::uint16_t(0);
	ASSERT_EQ(stream.total_write(), 3);
	stream << std::uint32_t(0);
	ASSERT_EQ(stream.total_write(), 7);
	stream << std::uint64_t(0);
	ASSERT_EQ(stream.total_write(), 15);

	std::string_view str { "hello, world!" };
	stream << hexi::raw(str);
	ASSERT_EQ(stream.total_write(), 28);
	stream << hexi::prefixed(str);
	ASSERT_EQ(stream.total_write(), 45);
	stream << hexi::prefixed_varint(str);
	ASSERT_EQ(stream.total_write(), 59);
	stream << hexi::null_terminated(str);
	ASSERT_EQ(stream.total_write(), 73);

	stream.put<std::uint8_t>(0);
	ASSERT_EQ(stream.total_write(), 74);
	stream.put<std::uint16_t>(0);
	ASSERT_EQ(stream.total_write(), 76);
	stream.put<std::uint32_t>(0);
	ASSERT_EQ(stream.total_write(), 80);
	stream.put<std::uint64_t>(0);
	ASSERT_EQ(stream.total_write(), 88);

	std::array<std::uint32_t, 4> data {};
	stream.put(data);
	ASSERT_EQ(stream.total_write(), 104);
	stream.put(data.data(), data.size());
	ASSERT_EQ(stream.total_write(), 120);
	stream.put(data.begin(), data.end());
	ASSERT_EQ(stream.total_write(), 136);
}
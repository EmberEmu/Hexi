#define HEXI_WITH_BOOST_ASIO // HEXI_WITH_ASIO for standalone Asio
#include <hexi.h>
#include <utility>
#include <vector>
#include <cstdint>

int main() {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);

	{ // serialise foo & bar as big/little endian
		const std::uint64_t foo = 100;
		const std::uint32_t bar = 200;
		stream.put<hexi::endian::conversion::native_to_big>(foo);
		stream.put<hexi::endian::conversion::native_to_little>(bar);
	}

	{ // deserialise foo & bar as big/little endian
		std::uint64_t foo = 0;

		// write to existing variable or return result
		stream.get<hexi::endian::conversion::big_to_native>(foo);
		std::ignore = stream.get<std::uint32_t, hexi::endian::conversion::little_to_native>();
	}

	{ // stream integers as various endian combinations
		stream << hexi::endian::native_to_big(9000);
		stream << hexi::endian::big_to_native(9001); // over 9000
		stream << hexi::endian::native_to_little(9002);
		stream << hexi::endian::little_to_native(9003);
	}

	{ // convert endianness inplace
		int foo = 10;
		hexi::endian::native_to_big_inplace(foo);
		hexi::endian::big_to_native_inplace(foo);
		hexi::endian::little_to_native_inplace(foo);
		hexi::endian::native_to_little_inplace(foo);
	}
}
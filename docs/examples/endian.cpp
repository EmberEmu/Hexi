#define HEXI_WITH_BOOST_ASIO // HEXI_WITH_ASIO for standalone Asio
#include <hexi.h>
#include <utility>
#include <vector>
#include <cstdint>

int main() {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor); // performs no conversions by default
	
	{
		hexi::binary_stream be_stream(adaptor, hexi::endian::big);    // will convert to/from big endian by default
		hexi::binary_stream be_stream(adaptor, hexi::endian::little); // will convert to/from little endian by default
		hexi::binary_stream be_stream(adaptor, hexi::endian::native); // default, will not convert by default
	}

	{ // serialise foo & bar as big/little endian with put
		const std::uint64_t foo = 100;
		const std::uint32_t bar = 200;
		stream.put<hexi::endian::to_big>(foo);
		stream.put<hexi::endian::to_little>(bar);
	}

	{ // deserialise foo & bar as big/little endian
		std::uint64_t foo = 0;

		// write to existing variable or return result
		stream.get<hexi::endian::to_big>(foo);
		std::ignore = stream.get<std::uint32_t, hexi::endian::conversion::little_to_native>();
	}

	{ // stream integers as various endian combinations
		stream << hexi::endian::to_big(9000);
		stream << hexi::endian::to_little(9001); // over 9000
		stream << hexi::endian::native_to_little(9002);
		stream << hexi::endian::little_to_native(9003);
	}

	{ // retrieve stream integers as big or little endian
		std::uint64_t foo;
		stream >> hexi::endian::from_big(foo);
		stream >> hexi::endian::from_little(foo);
	}

	{ // convert endianness inplace
		int foo = 10;
		hexi::endian::native_to_big_inplace(foo);
		hexi::endian::big_to_native_inplace(foo);
		hexi::endian::little_to_native_inplace(foo);
		hexi::endian::native_to_little_inplace(foo);
	}

	{ // retrieve converted value
		auto foo = hexi::endian::native_to_big_inplace(1);
		auto bar = hexi::endian::big_to_native_inplace(2);
		auto baz = hexi::endian::little_to_native_inplace(3);
		auto qux = hexi::endian::native_to_little(4);
	}
}
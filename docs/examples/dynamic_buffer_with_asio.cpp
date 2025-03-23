#define HEXI_WITH_BOOST_ASIO // HEXI_WITH_ASIO for standalone Asio
#include <hexi.h>
#include <boost/asio.hpp>
#include <array>
#include <numeric>
#include <vector>

constexpr auto buffer_block_size = 128uz;

int main() {
	boost::asio::io_context ctx;
	boost::asio::ip::tcp::socket socket(ctx);

	// connect the socket and so on

	hexi::dynamic_buffer<buffer_block_size> buffer;
	hexi::binary_stream stream(buffer);

	// write a bunch of dummy data to the buffer
	for (auto i = 0; i < 10'000; ++i) {
		stream << i;
	}

	// give Asio a lightweight copyable view of the buffer
	const hexi::buffer_sequence sequence(buffer);
	socket.send(sequence);
}
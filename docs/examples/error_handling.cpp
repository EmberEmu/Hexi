#include <hexi.h>
#include <print>
#include <span>
#include <vector>
#include <cstddef>

void error_codes(std::span<const std::byte> buffer) {
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream<decltype(adaptor), hexi::no_throw> stream(adaptor);

	// intentionally trigger an error by trying to read out of bounds
	stream.skip(buffer.size() + 1);

	if (stream) {
		std::println("This shouldn't have worked");
	} else {
		std::println("Stream error, {}", std::to_underlying(stream.state()));
	}
}

void exceptions(std::span<const std::byte> buffer) try {
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);

	// intentionally trigger an error by trying to read out of bounds
	stream.skip(buffer.size() + 1);
} catch (const hexi::exception& e) {
	std::cout << e.what();
}

int main() {
	std::vector<int> buffer { 1, 2, 3, 4, 5 };
	auto bytes = std::as_bytes(std::span(buffer));

	// let's try to break stuff!
	error_codes(bytes);
	exceptions(bytes);
}
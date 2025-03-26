#include <hexi.h>
#include <print>
#include <vector>
#include <cstdlib>

struct Data {
	uint8_t value = 0; // only powers of two are valid values in this example

	auto& operator<<(auto& stream) {
		if (value % 2 != 0) { // error if value is not a power of two
			stream.set_error_state();
			return stream;
		}

		stream << value;
		return stream;
	}
};

int main() {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);

	Data foo {
		.value = 2
	};

	stream << foo;

	if (!stream) {
		std::print("Serialisation failed, should not happen here");
		return EXIT_FAILURE;
	}

	foo.value = 3;

	stream << foo;

	if (!stream) {
		std::print("Serialisation failed successfully!");
		return EXIT_SUCCESS;
	}

	return EXIT_FAILURE;
}
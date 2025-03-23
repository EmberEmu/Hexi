#include <hexi.h>
#include <array>
#include <span>
#include <vector>

int main() {
	/**
	* std buffer types
	*/

	{ // read-only std::span
		std::vector<char> buffer;
		std::span span(buffer);
		hexi::buffer_adaptor adaptor(span);
		hexi::binary_stream stream(adaptor);
	}

	{ // read-write std::string
		std::string buffer;
		hexi::buffer_adaptor adaptor(buffer);
		hexi::binary_stream stream(adaptor);
	}

	{ // read-write std::vector
		std::vector<char> buffer;
		hexi::buffer_adaptor adaptor(buffer);
		hexi::binary_stream stream(adaptor);
	}

	{ // read-only std::string_view
		std::string_view buffer;
		hexi::buffer_adaptor adaptor(buffer);
		hexi::binary_stream stream(adaptor);
	}

	{ // read-only std::array
		std::array<char, 10> buffer;
		hexi::buffer_adaptor adaptor(buffer);
		hexi::binary_stream stream(adaptor);
	}

	/**
	* hexi buffer types
	*/

	{ // read-write hexi::static_buffer
		hexi::static_buffer<char, 10> buffer;
		hexi::binary_stream stream(buffer);
	}

	{ // read-write hexi::dynamic_buffer
		hexi::dynamic_buffer<128> buffer;
		hexi::binary_stream stream(buffer);
	}

	{ // read-write hexi::file_buffer
		hexi::file_buffer buffer("file.tmp");
		hexi::binary_stream stream(buffer);
	}
}
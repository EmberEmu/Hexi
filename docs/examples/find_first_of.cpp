#include <hexi.h>
#include <string>
#include <string_view>
#include <vector>

int main() {
	std::string str { "The quick brown fox jumped over the lazy dog"};

	// create stream
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);

	// serialise our string
	stream << str;

	// write a sentinel value
	stream << 0xff;

	// search the buffer for the sentinel value
	const auto pos = adaptor.find_first_of(0xff);

	if (pos != adaptor.npos) {
		std::print("Found it!");
	} else {
		std::print("Where did it go?");
	}
}
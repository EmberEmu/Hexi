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

	// read it back but into a string_view, no allocations, no copying!
	std::string_view view;
	stream >> view;

	if (str == view) {
		std::print("It matched! :)");
	} else {
		std::print("Something went awry... :()");
	}
}
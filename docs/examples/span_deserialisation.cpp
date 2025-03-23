#include <hexi.h>
#include <algorithm>
#include <span>
#include <string_view>
#include <vector>
#include <cstdlib>

int main() {
	// put some random data into a vector
	std::vector<int> random_data(10);
	std::ranges::generate(random_data, rand);

	// create buffer & stream
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);

	// write random ints to the stream
	stream.put(random_data);

	// get a view over the data - nearly free deserialisation!
	auto span = stream.span<int>(10);

	if (std::ranges::equal(span, random_data)) {
		std::print("Great!");
	} else {
		std::print("Uh oh, something isn't right.");
	}
}
#include <hexi.h>
#include <string>
#include <string_view>
#include <vector>

/**
 * Hexi provides string adaptors that allow for control over how std::string and
 * std::string_views are serialised. This is to ensure the element of least surprise
 * in the case of handling strings that contain embedded null bytes.
 * 
 * raw: serialise the string exactly as-is
 * 
 * null_terminated: ensure the string is null-terminated. Strings with null bytes
 * will trigger an assert in debug builds. This is to ensure that the input of
 * a null_terminated serialisation exactly matches the output.
 * 
 * prefixed: writes the length of the string before the string's data. These strings
 * can contain embedded null bytes.
 * 
 * prefixed_varint: writes the length of the string before the string's data with a compact
 * varint coding. This is slower than prefixed but more compact.
 * These strings can contain embedded null bytes.
 * 
 * const char* strings will always be written as null terminated strings and should be
 * retrieved as such.
 * 
 * If no adaptor is provided, prefixed will be implicitly used. This is to ensure that
 * the output will always match the input even in cases of embedded nulls.
 */
int main() {
	std::string str { "The quick brown fox jumped over the lazy dog"};

	// create stream
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);

	// serialise our string - will be prefixed by default
	stream << str;

	// serialise our string with null_terminated, will always be null terminated
	stream << hexi::null_terminated(str);

	// serialise our string by length prefixing
	stream << hexi::prefixed(str);

	// serialise our string by prefixing with a length varint
	stream << hexi::prefixed_varint(str);

	// serialise our string with raw, will not be terminated
	stream << hexi::raw(str);

	// read our strings back
	std::string_view view;
	stream >> view;
	stream >> hexi::null_terminated(view);
	stream >> hexi::prefixed(view);
	stream >> hexi::prefixed_varint(view);

	// retrieving a raw string is more complex - we need to know the size
	// alternatively, we can write a terminating/sentinel byte manually and
	// call find_first_of to determine length or to get a string_view with
	// a call to stream.view();
	std::string output;
	output.resize(output.size());
	stream.get(output.data(), output.size());
}
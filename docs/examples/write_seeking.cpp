#include <hexi.h>
#include <print>
#include <string>
#include <vector>
#include <cstdint>
#include <ctime>

struct UserPacket {
	uint64_t user_id;
	std::string username;
	int64_t timestamp;
	uint8_t has_optional_field;
	uint32_t optional_field;

	auto& operator>>(auto& stream) {
		stream >> user_id >> username >> timestamp >> has_optional_field;

		if (has_optional_field) {
			stream >> optional_field;
		}

		return stream;
	}

	auto& operator<<(auto& stream) const {
		stream << user_id << username << timestamp << has_optional_field;

		if (has_optional_field) {
			stream << optional_field;
		}

		return stream;
	}

	bool operator==(const UserPacket& rhs) const {
		return user_id == rhs.user_id
			&& username == rhs.username
			&& timestamp == rhs.timestamp
			&& has_optional_field == rhs.has_optional_field
			&& optional_field == rhs.optional_field;
	}
};

int main() {
	std::vector<char> buffer;
	hexi::buffer_adaptor adaptor(buffer);
	hexi::binary_stream stream(adaptor);

	UserPacket packet_in {
		.user_id = 0,
		.username = "Administrator",
		.timestamp = time(NULL),
		.has_optional_field = 1,
		.optional_field = 9001
	};

	// reserve bytes at the start of the stream & serialise
	stream << std::size_t(0) << packet_in;

	// get number of bytes written
	std::size_t written = stream.size();

	// move write cursor back to the beginning
	stream.write_seek(hexi::stream_seek::sk_stream_absolute, 0);

	// write packet size to the beginning of the stream
	stream << written;

	// write cursor must be put back before reading
	stream.write_seek(hexi::stream_seek::sk_stream_absolute, written);

	// pretend to deserialise
	std::size_t read_size{};
	UserPacket packet_out;
	stream >> read_size >> packet_out;

	if (read_size == written && packet_in == packet_out) {
		std::print("Everything worked!");
	} else {
		std::printf("Something went wrong...");
	}
}
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

	/**
	 * If serialisation and deserialisation logic differs, we can also
	 * define operator<< and operator>> for use with stream << type
	 * and stream >> type
	 */
	void serialise(auto& stream) {
		stream(user_id, username, timestamp, has_optional_field);

		if (has_optional_field) {
			stream(optional_field);
			
			// can also use this syntax:
			// stream & optional_field;
		}
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

	// serialise
	stream.serialise(packet_in);

	// round-trip deserialise
	UserPacket packet_out{};
	stream.deserialise(packet_out);

	if(packet_in == packet_out) {
		std::print("Everything went great!");
	} else {
		std::print("Something went wrong!");
	}
}
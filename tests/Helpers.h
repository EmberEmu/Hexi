//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#include <filesystem>
#include <fstream>
#include <vector>
#include <stdexcept>

static std::vector<char> read_file(const std::filesystem::path& path) {
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	const auto pos = file.tellg();

	if(!file) {
		throw std::runtime_error("File open failed");
	}

	if(!pos) {
		return{};
	}

	std::vector<char> buffer(pos);
	file.seekg(0, std::ios::beg);
	file.read(buffer.data(), buffer.size());

	if(!file) {
		throw std::runtime_error("File read failed");
	}

	return buffer;
}
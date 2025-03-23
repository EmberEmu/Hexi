//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <format>
#include <stdexcept>
#include <cstddef>

namespace hexi {

class exception : public std::runtime_error {
public:
	exception(const std::string& msg)
		: std::runtime_error(msg) {}
};

class buffer_underrun final : public exception {
public:
	const std::size_t buff_size, read_size, total_read;

	buffer_underrun(std::size_t read_size, std::size_t total_read, std::size_t buff_size)
		: exception(std::format(
			"Buffer underrun: {} byte read requested, buffer contains {} bytes and total bytes read was {}",
			read_size, buff_size, total_read)),
		buff_size(buff_size), read_size(read_size), total_read(total_read) {}
};

class buffer_overflow final : public exception {
public:
	const std::size_t free, write_size, total_write;

	buffer_overflow(std::size_t write_size, std::size_t total_write, std::size_t free)
		: exception(std::format(
			"Buffer overflow: {} byte write requested, free space is {} bytes and total bytes written was {}",
			write_size, free, total_write)),
		free(free), write_size(write_size), total_write(total_write) {}
};

class stream_read_limit final : public exception {
public:
	const std::size_t read_limit, read_size, total_read;

	stream_read_limit(std::size_t read_size, std::size_t total_read, std::size_t read_limit)
		: exception(std::format(
			"Read boundary exceeded: {} byte read requested, read limit was {} bytes and total bytes read was {}",
			read_size, read_limit, total_read)),
		read_limit(read_limit), read_size(read_size), total_read(total_read) {}
};

} // hexi

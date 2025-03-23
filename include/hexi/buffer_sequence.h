//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#if defined HEXI_WITH_ASIO || defined HEXI_WITH_BOOST_ASIO

#ifdef HEXI_WITH_ASIO
#include <asio/buffer.hpp>
#elif defined HEXI_WITH_BOOST_ASIO
#include <boost/asio/buffer.hpp>
#endif

#ifdef BUFFER_DEBUG
#include <span>
#endif

namespace hexi {

#ifdef HEXI_WITH_BOOST_ASIO
namespace asio = boost::asio;
#endif

template<typename buffer_type>
class buffer_sequence {
	const buffer_type& buffer_;

public:
	buffer_sequence(const buffer_type& buffer)
		: buffer_(buffer) { }

	class const_iterator {
		using Node = typename buffer_type::node_type;

	public:
		const_iterator(const buffer_type& buffer, const Node* curr_node)
			: buffer_(buffer),
			  curr_node_(curr_node) {}

		const_iterator& operator++() {
			curr_node_ = curr_node_->next;
			return *this;
		}

		const_iterator operator++(int) {
			const_iterator current(*this);
			curr_node_ = curr_node_->next;
			return current;
		}

		asio::const_buffer operator*() const {
			const auto buffer = buffer_.buffer_from_node(curr_node_);
			return asio::const_buffer(buffer->read_data(), buffer->size());
		}

		bool operator==(const const_iterator& rhs) const {
			return curr_node_ == rhs.curr_node_;
		}

		bool operator!=(const const_iterator& rhs) const {
			return curr_node_ != rhs.curr_node_;
		}

		const_iterator& operator=(const_iterator&) = delete;

	#ifdef BUFFER_DEBUG
		std::span<const char> get_buffer() {
			auto buffer = buffer_.buffer_from_node(curr_node_);
			return {
				reinterpret_cast<const char*>(buffer->read_data()), buffer->size()
			};
		}
	#endif

	private:
		const buffer_type& buffer_;
		const Node* curr_node_;
	};

const_iterator begin() const {
	return const_iterator(buffer_, buffer_.root_.next);
}

const_iterator end() const {
	return const_iterator(buffer_, &buffer_.root_);
}

friend class const_iterator;
};

} // hexi

#endif // #if defined HEXI_WITH_ASIO || defined HEXI_WITH_BOOST_ASIO
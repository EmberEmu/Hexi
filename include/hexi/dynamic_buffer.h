//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/pmc/buffer.h>
#include <hexi/shared.h>
#include <hexi/allocators/default_allocator.h>
#include <hexi/detail/intrusive_storage.h>
#include <concepts>
#include <functional>
#include <memory>
#include <utility>
#ifdef HEXI_BUFFER_DEBUG
#include <algorithm>
#include <vector>
#endif
#include <cstddef>
#include <cstdint>
#include <cassert>

namespace hexi {

using namespace detail;

template<typename buffer_type>
class buffer_sequence;

template<decltype(auto) block_sz>
concept int_gt_zero = std::integral<decltype(block_sz)> && block_sz > 0;

template<decltype(auto) block_sz,
	byte_type storage_value_type = std::byte,
	typename allocator = default_allocator<detail::intrusive_storage<block_sz, storage_value_type>>
>
requires int_gt_zero<block_sz>
class dynamic_buffer final : public pmc::buffer {
public:
	using storage_type = intrusive_storage<block_sz, storage_value_type>;
	using value_type   = storage_value_type;
	using node_type    = intrusive_node;
	using size_type    = std::size_t;
	using offset_type  = std::size_t;
	using contiguous   = is_non_contiguous;
	using seeking      = supported;

	static constexpr auto npos { static_cast<size_type>(-1) };

	using unique_storage = std::unique_ptr<storage_type, std::function<void(storage_type*)>>;

private:
	intrusive_node root_;
	size_type size_;
	[[no_unique_address]] allocator allocator_;

	void link_tail_node(intrusive_node* node) {
		node->next = &root_;
		node->prev = root_.prev;
		root_.prev = root_.prev->next = node;
	}

	void unlink_node(intrusive_node* node) {
		node->next->prev = node->prev;
		node->prev->next = node->next;
	}

	inline storage_type* buffer_from_node(const intrusive_node* node) const {
		return reinterpret_cast<storage_type*>(std::uintptr_t(node)
			- offsetof(storage_type, node));
	}

	void move(dynamic_buffer& rhs) noexcept {
		if(this == &rhs) { // self-assignment
			return;
		}

		clear(); // clear our current blocks rather than swapping them

		size_ = rhs.size_;
		root_ = rhs.root_;
		root_.next->prev = &root_;
		root_.prev->next = &root_;
		rhs.size_ = 0;
		rhs.root_.next = &rhs.root_;
		rhs.root_.prev = &rhs.root_;
	}

	void copy(const dynamic_buffer& rhs) {
		if(this == &rhs) { // self-assignment
			return;
		}

		const intrusive_node* head = rhs.root_.next;
		root_.next = &root_;
		root_.prev = &root_;
		size_ = 0;

		while(head != &rhs.root_) {
			auto buffer = allocate();
			*buffer = *buffer_from_node(head);
			link_tail_node(&buffer->node);
			size_ += buffer->write_offset;
			head = head->next;
		}
	}
	
#ifdef HEXI_BUFFER_DEBUG
	void offset_buffers(std::vector<storage_type*>& buffers, size_type offset) {
		std::erase_if(buffers, [&](auto block) {
			if(block->size() > offset) {
				block->read_offset += offset;
				block->write_offset -= offset;
				return false;
			} else {
				return true;
			}
		});
	}
#endif

	value_type& byte_at_index(const size_type index) const {
		assert(index < size_ && "buffer subscript index out of range");

		auto head = root_.next;
		auto buffer = buffer_from_node(head);
		const auto offset_index = index + buffer->read_offset;
		const auto node_index = offset_index / block_sz;

		for(size_type i = 0; i < node_index; ++i) {
			head = head->next;
		}

		buffer = buffer_from_node(head);
		return (*buffer)[offset_index % block_sz];
	}

	size_type abs_seek_offset(size_type offset) {
		if(offset < size_) {
			return size_ - offset;
		} else if(offset > size_) {
			return offset - size_;
		} else {
			return 0;
		}
	}

	[[nodiscard]] storage_type* allocate() {
		return allocator_.allocate();
	}

	void deallocate(storage_type* buffer) {
		allocator_.deallocate(buffer);
	}

public:
	dynamic_buffer()
		: root_{ .next = &root_, .prev = &root_ },
		  size_(0) {}

	~dynamic_buffer() {
		clear();
	}

	dynamic_buffer& operator=(dynamic_buffer&& rhs) noexcept {
		move(rhs);
		return *this;
	}

	dynamic_buffer(dynamic_buffer&& rhs) noexcept {
		move(rhs);
	}

	dynamic_buffer(const dynamic_buffer& rhs) {
		copy(rhs);
	}

	dynamic_buffer& operator=(const dynamic_buffer& rhs) {
		clear();
		copy(rhs);
		return *this;
	}

	template<typename T>
	void read(T* destination) {
		read(destination, sizeof(T));
	}

	void read(void* destination, size_type length) override {
		assert(length <= size_ && "Chained buffer read too large!");
		size_type remaining = length;

		while(true) {
			auto buffer = buffer_from_node(root_.next);
			remaining -= buffer->read(
				static_cast<value_type*>(destination) + length - remaining, remaining,
				                         root_.next == root_.prev
			);

			if(remaining) [[unlikely]] {
				unlink_node(root_.next);
				deallocate(buffer);
			} else {
				break;
			}
		}

		size_ -= length;
	}

	template<typename T>
	void copy(T* destination) const {
		copy(destination, sizeof(T));
	}

	void copy(void* destination, const size_type length) const override {
		assert(length <= size_ && "Chained buffer copy too large!");
		size_type remaining = length;
		auto head = root_.next;

		while(true) {
			const auto buffer = buffer_from_node(head);
			remaining -= buffer->copy(
				static_cast<value_type*>(destination) + length - remaining, remaining
			);

			if(remaining) [[unlikely]] {
				head = head->next;
			} else {
				break;
			}
		}
	}

#ifdef HEXI_BUFFER_DEBUG
	std::vector<storage_type*> fetch_buffers(const size_type length, const size_type offset = 0) {
		size_type total = length + offset;
		assert(total <= size_ && "Chained buffer fetch too large!");
		std::vector<storage_type*> buffers;
		auto head = root_.next;

		while(total) {
			auto buffer = buffer_from_node(head);
			size_type read_size = block_sz - buffer->read_offset;
			
			// guard against overflow - buffer may have more content than requested
			if(read_size > total) {
				read_size = total;
			}

			buffers.emplace_back(buffer);
			total -= read_size;
			head = head->next;
		}

		if(offset) {
			offset_buffers(buffers, offset);
		}

		return buffers;
	}
#endif

	void skip(const size_type length) override {
		assert(length <= size_ && "Chained buffer skip too large!");
		size_type remaining = length;

		while(true) {
			auto buffer = buffer_from_node(root_.next);
			remaining -= buffer->skip(remaining, root_.next == root_.prev);

			if(remaining) [[unlikely]] {
				unlink_node(root_.next);
				deallocate(buffer);
			} else {
				break;
			}
		}

		size_ -= length;
	}

	void write(const auto& source) {
		write(&source, sizeof(source));
	}

	void write(const void* source, const size_type length) override {
		size_type remaining = length;
		intrusive_node* tail = root_.prev;

		do {
			storage_type* buffer;

			if(tail != &root_) [[likely]] {
				buffer = buffer_from_node(tail);
			} else {
				buffer = allocate();
				link_tail_node(&buffer->node);
				tail = root_.prev;
			}

			remaining -= buffer->write(
				static_cast<const value_type*>(source) + length - remaining, remaining
			);

			tail = tail->next;
		} while(remaining);

		size_ += length;
	}

	void reserve(const size_type length) override {
		size_type remaining = length;
		intrusive_node* tail = root_.prev;

		do {
			storage_type* buffer;

			if(tail == &root_) [[unlikely]] {
				buffer = allocate();
				link_tail_node(&buffer->node);
				tail = root_.prev;
			} else {
				buffer = buffer_from_node(tail);
			}

			remaining -= buffer->advance_write(remaining);
			tail = tail->next;
		} while(remaining);

		size_ += length;
	}

	size_type size() const override {
		return size_;
	}

	storage_type* back() const {
		if(root_.prev == &root_) {
			return nullptr;
		}

		return buffer_from_node(root_.prev);
	}

	storage_type* front() const {
		if(root_.next == &root_) {
			return nullptr;
		}

		return buffer_from_node(root_.next);
	}

	auto pop_front() {
		auto buffer = buffer_from_node(root_.next);
		size_ -= buffer->size();
		unlink_node(root_.next);
		return unique_storage(buffer, [&](auto ptr) {
			deallocate(ptr);
		});
	}

	void push_back(storage_type* buffer) {
		link_tail_node(&buffer->node);
		size_ += buffer->write_offset;
	}

	void advance_write(const size_type size) {
		auto buffer = buffer_from_node(root_.prev);
		const auto actual = buffer->advance_write(size);
		assert(size <= block_sz && actual <= size &&
		       "Attempted to advance write cursor out of bounds!");
		size_ += size;
	}

	bool can_write_seek() const override {
		return std::is_same_v<seeking, supported>;
	}

	void write_seek(const buffer_seek mode, size_type offset) override {
		// nothing to do in this case
		if(mode == buffer_seek::sk_absolute && offset == size_) {
			return;
		}

		auto tail = root_.prev;

		switch(mode) {
			case buffer_seek::sk_backward:
				size_ -= offset;
				break;
			case buffer_seek::sk_forward:
				size_ += offset;
				break;
			case buffer_seek::sk_absolute:
				size_ = offset;
				offset = abs_seek_offset(offset);
				break;
		}

		const bool rewind = (mode == buffer_seek::sk_backward
							 || (mode == buffer_seek::sk_absolute && offset < size_));

		while(offset) {
			auto buffer = buffer_from_node(tail);
			const auto max_seek = rewind? buffer->size() : buffer->free();

			if(max_seek >= offset) {
				buffer->write_seek(mode, offset);
				offset = 0;
			} else {
				buffer->write_seek(mode, max_seek);
				offset -= max_seek;
				tail = rewind? tail->prev : tail->next;
			}
		}

		root_.prev = tail;
	}

	void clear() {
		intrusive_node* head = root_.next;

		while(head != &root_) {
			auto next = head->next;
			deallocate(buffer_from_node(head));
			head = next;
		}

		root_.next = &root_;
		root_.prev = &root_;
		size_ = 0;
	}

	[[nodiscard]]
	bool empty() const override {
		return !size_;
	}
	
	constexpr static size_type block_size() {
		return block_sz;
	}

	value_type& operator[](const size_type index) override {
		return byte_at_index(index);
	}

	const value_type& operator[](const size_type index) const override {
		return byte_at_index(index);
	}

	size_type block_count() {
		auto node = &root_;
		size_type count = 0;

		// not calculating based on block size & size as it
		// wouldn't play nice with seeking or manual push/pop
		while(node->next != root_.prev->next) {
			++count;
			node = node->next;
		}

		return count;
	}

	size_type find_first_of(value_type val) const override {
		size_type index = 0;
		auto head = root_.next;

		while(head != &root_) {
			const auto buffer = buffer_from_node(head);
			const auto data = buffer->read_data();
			
			for(size_type i = 0, j = buffer->size(); i < j; ++i, ++index) {
				if(data[i] == val) {
					return index;
				}
			}

			head = head->next;
		}

		return npos;
	}

	auto& get_allocator() {
		return allocator_;
	}

	const auto& get_allocator() const {
		return allocator_;
	}

	template<typename buffer_type>
	friend class buffer_sequence;
};

} // hexi

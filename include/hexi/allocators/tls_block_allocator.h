//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/allocators/block_allocator.h>
#include <type_traits>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstdint>

#ifndef NDEBUG
#define HEXI_DEBUG_ALLOCATORS
#endif

namespace hexi {

struct safe_entrant {};
struct no_ref_counting {};

struct unsafe_entrant : safe_entrant {};
struct ref_counting : no_ref_counting {};

template<typename _ty,
	std::size_t _elements,
	std::derived_from<no_ref_counting> ref_count_policy = no_ref_counting,
	std::derived_from<safe_entrant> entrant_policy = safe_entrant
>
class tls_block_allocator final {
	using allocator_type = block_allocator<_ty, _elements>;

	using ref_count = std::conditional_t<
		std::is_same_v<ref_count_policy, ref_counting>, int, std::monostate
	>;

	using tls_handle_cache = std::conditional_t<
		std::is_same_v<entrant_policy, unsafe_entrant>, allocator_type*, std::monostate
	>;

	static inline thread_local std::unique_ptr<allocator_type> allocator_;
	static inline thread_local ref_count ref_count_{};

	[[no_unique_address]] tls_handle_cache cached_handle_{};

	// Compiler will optimise calls to this out when using unsafe_entrant
	inline void initialise() {
		if constexpr(std::is_same_v<entrant_policy, safe_entrant>) {
			if(!allocator_) {
				allocator_ = std::make_unique<allocator_type>();
			}
		}
	}

	inline allocator_type* allocator_handle() {
		if constexpr(std::is_same_v<entrant_policy, unsafe_entrant>) {
			return cached_handle_;
		} else {
			return allocator_.get();
		}
	}

public:
#ifdef HEXI_DEBUG_ALLOCATORS
	std::size_t total_allocs = 0;
	std::size_t total_deallocs = 0;
	std::size_t active_allocs = 0;
#endif

	tls_block_allocator() {
		thread_enter();
	}

	/*
	 * When used in conjunction with unsafe_entrant, allows the owning object
	 * to be executed on another thread without paying for checks on every
	 * allocation
	 */
	inline void thread_enter() {
		if(!allocator_) {
			allocator_ = std::make_unique<allocator_type>();
		}

		if constexpr(std::is_same_v<entrant_policy, unsafe_entrant>) {
			cached_handle_ = allocator_.get();
		}

		if constexpr(std::is_same_v<ref_count_policy, ref_counting>) {
			++ref_count_;
		}
	}

	inline void thread_exit() {
		if constexpr(std::is_same_v<ref_count_policy, ref_counting>) {
			assert(ref_count_);

			--ref_count_;

			if(ref_count_ == 0) {
				allocator_.reset();
			}
		}
	}

	template<typename ...Args>
	[[nodiscard]] inline _ty* allocate(Args&&... args) {
		/*
		 * When safe_entrant is set, need to do this here & in ctor unless
		 * we can be 100% sure that any object using the allocator is created
		 * on the same thread that ends up using it.
		 */
		initialise();

#ifdef HEXI_DEBUG_ALLOCATORS
		++total_allocs;
		++active_allocs;
#endif
		return allocator_handle()->allocate(std::forward<Args>(args)...);
	}

	inline void deallocate(_ty* t) {
		assert(t);

#ifdef HEXI_DEBUG_ALLOCATORS
		++total_deallocs;
		--active_allocs;
#endif
		allocator_handle()->deallocate(t);
	}

#ifdef HEXI_DEBUG_ALLOCATORS
	auto allocator() {
		initialise();
		return allocator_handle();
	}
#endif

	~tls_block_allocator() {
		thread_exit();

#ifdef HEXI_DEBUG_ALLOCATORS
		assert(active_allocs == 0);
#endif
	}
};

} // hexi

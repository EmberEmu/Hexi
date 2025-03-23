//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <hexi/dynamic_buffer.h>
#include <hexi/allocators/tls_block_allocator.h>
#include <cstddef>

namespace hexi {

// dynamic_buffer backed by thread-local storage, meaning every
// instance on the same thread shares the same underlying memory.
// As a rule of thumb, an instance should never be touched by any thread
// other than the one on which it was created, not even if synchronised
// ... unless you're positive it won't result in the allocator being called.
// 
// Minimum memory usage is intrusive_storage<block_size> * count.
// Additional blocks are not added if the original is exhausted ('colony' structure),
// so the allocator will fall back to the system allocator instead.
//
// Pros: extremely fast allocation/deallocation for many instances per thread
// Cons: everything else.
// 
// TL;DR Do not use unless you know what you're doing.
template<decltype(auto) block_size,
	std::size_t count,
	typename ref_count_policy = no_ref_counting,
	typename entrant_policy = safe_entrant,
	typename storage_type = std::byte>
using dynamic_tls_buffer = dynamic_buffer<block_size, storage_type,
	tls_block_allocator<typename dynamic_buffer<block_size>::storage_type, count, no_ref_counting, entrant_policy>
>;

} // hexi

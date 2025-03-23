//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#include <gtest/gtest.h>
#include <array>
#include <chrono>
#include <thread>
#include <cstdlib>

#define HEXI_DEBUG_ALLOCATORS
#include <hexi/allocators/tls_block_allocator.h>

TEST(tls_block_allocator, single_alloc) {
	hexi::tls_block_allocator<std::uint64_t, 1> tlsalloc;
	auto mem = tlsalloc.allocate();
	ASSERT_EQ(tlsalloc.allocator()->storage_active_count, 1);
	ASSERT_EQ(tlsalloc.allocator()->new_active_count, 0);
	ASSERT_EQ(tlsalloc.total_allocs, 1);
	ASSERT_EQ(tlsalloc.total_deallocs, 0);
	tlsalloc.deallocate(mem);
	ASSERT_EQ(tlsalloc.allocator()->storage_active_count, 0);
	ASSERT_EQ(tlsalloc.allocator()->new_active_count, 0);
	ASSERT_EQ(tlsalloc.total_allocs, 1);
	ASSERT_EQ(tlsalloc.total_deallocs, 1);
}

TEST(tls_block_allocator, random_allocs) {
	const auto max_allocs = 100u;
	hexi::tls_block_allocator<std::uint64_t, max_allocs> tlsalloc;
	std::array<std::uint64_t*, max_allocs> chunks{};
	const auto time = std::chrono::system_clock::now().time_since_epoch();
	const unsigned int seed = static_cast<unsigned int>(time.count());
	std::srand(seed);
	const auto allocs = std::rand() % max_allocs;
	const auto tls_total_alloc = tlsalloc.allocator()->total_allocs;
	const auto tls_total_dealloc = tlsalloc.allocator()->total_deallocs;

	for(std::size_t i = 0u; i < allocs; ++i) {
		auto mem = tlsalloc.allocate();
		chunks[i] = mem;
	}

	ASSERT_EQ(tlsalloc.total_allocs, allocs);
	ASSERT_EQ(tlsalloc.active_allocs, allocs);
	ASSERT_EQ(tlsalloc.total_deallocs, 0);
	ASSERT_EQ(tlsalloc.allocator()->total_allocs, tls_total_alloc + allocs);
	ASSERT_EQ(tlsalloc.allocator()->total_deallocs, tls_total_dealloc);

	for(std::size_t i = 0u; i < allocs; ++i) {
		tlsalloc.deallocate(chunks[i]);
	}

	ASSERT_EQ(tlsalloc.total_allocs, allocs);
	ASSERT_EQ(tlsalloc.active_allocs, 0);
	ASSERT_EQ(tlsalloc.total_deallocs, allocs);
	ASSERT_EQ(tlsalloc.allocator()->total_allocs, tls_total_alloc + allocs);
	ASSERT_EQ(tlsalloc.allocator()->total_deallocs, tls_total_dealloc + allocs);
}

TEST(tls_block_allocator, over_capacity) {
	hexi::tls_block_allocator<std::uint64_t, 1> tlsalloc;
	std::array<std::uint64_t*, 2> mem{};
	mem[0] = tlsalloc.allocate();
	mem[1] = tlsalloc.allocate();
	ASSERT_EQ(tlsalloc.allocator()->storage_active_count, 1);
	ASSERT_EQ(tlsalloc.allocator()->new_active_count, 1);
	ASSERT_EQ(tlsalloc.total_allocs, 2);
	ASSERT_EQ(tlsalloc.total_deallocs, 0);
	tlsalloc.deallocate(mem[0]);
	ASSERT_EQ(tlsalloc.allocator()->storage_active_count, 0);
	ASSERT_EQ(tlsalloc.allocator()->new_active_count, 1);
	ASSERT_EQ(tlsalloc.total_allocs, 2);
	ASSERT_EQ(tlsalloc.total_deallocs, 1);
	tlsalloc.deallocate(mem[1]);
	ASSERT_EQ(tlsalloc.allocator()->storage_active_count, 0);
	ASSERT_EQ(tlsalloc.allocator()->new_active_count, 0);
	ASSERT_EQ(tlsalloc.total_allocs, 2);
	ASSERT_EQ(tlsalloc.total_deallocs, 2);
}

TEST(tls_block_allocator, no_sharing) {
	hexi::tls_block_allocator<std::uint64_t, 2> tlsalloc;
	const auto tls_total_alloc = tlsalloc.allocator()->total_allocs;
	const auto tls_total_dealloc = tlsalloc.allocator()->total_deallocs;
	auto chunk = tlsalloc.allocate();
	ASSERT_EQ(tlsalloc.allocator()->storage_active_count, 1);
	ASSERT_EQ(tlsalloc.allocator()->total_allocs, tls_total_alloc + 1);

	std::thread thread([&] {
		hexi::tls_block_allocator<std::uint64_t, 2> tlsalloc_;
		ASSERT_EQ(tlsalloc_.allocator()->total_allocs, 0);
		ASSERT_EQ(tlsalloc_.allocator()->storage_active_count, 0);
		auto chunk_ = tlsalloc_.allocate();
		ASSERT_EQ(tlsalloc_.allocator()->storage_active_count, 1);
		ASSERT_EQ(tlsalloc_.allocator()->total_allocs, 1);
		ASSERT_EQ(tlsalloc_.allocator()->total_deallocs, 0);
		tlsalloc_.deallocate(chunk_);
		ASSERT_EQ(tlsalloc_.allocator()->total_deallocs, 1);
	});

	thread.join();

	tlsalloc.deallocate(chunk);
	ASSERT_EQ(tlsalloc.allocator()->total_deallocs, tls_total_dealloc + 1);
}

TEST(tls_block_allocator, thread_mismatch) {
	hexi::tls_block_allocator<std::uint64_t, 1> tlsalloc;
	auto chunk = tlsalloc.allocate();

	std::jthread thread([&] {
		ASSERT_DEATH(tlsalloc.deallocate(chunk), "");
	});

	// needed to stop further asserts from triggering
	tlsalloc.deallocate(chunk);
}
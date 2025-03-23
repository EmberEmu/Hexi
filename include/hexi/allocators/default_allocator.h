//  _               _ 
// | |__   _____  _(_)
// | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
// | | | |  __/>  <| | Version 1.0
// |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

#pragma once

#include <utility>

namespace hexi {

template<typename T>
struct default_allocator final {
	template<typename ...Args>
	[[nodiscard]] inline T* allocate(Args&&... args) const {
		return new T(std::forward<Args>(args)...);
	}

	inline void deallocate(T* t) const {
		delete t;
	}
};

} // hexi

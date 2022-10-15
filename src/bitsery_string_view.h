#pragma once

#include <string_view>
#include <bitsery/traits/core/std_defaults.h>

namespace bitsery {
	namespace traits {
		template<typename T, typename Traits>
		struct ContainerTraits<std::basic_string_view<T, Traits>>:
				public StdContainer<std::basic_string_view<T, Traits>, false, true> {
		};
		
		template<typename T, typename Traits>
		struct BufferAdapterTraits<std::basic_string_view<T, Traits>>:
				public StdContainerForBufferAdapter<std::basic_string_view<T, Traits>> {
		};
		
	}
	
}

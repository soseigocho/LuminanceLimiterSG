#pragma once

#include <aviutl/filter.hpp>

#include <algorithm>
#include <concepts>
#include <vector>


namespace luminance_limiter_sg {
	using NormalizedY = float;
	class NormalizedYBuffer {
	public:
		NormalizedYBuffer(AviUtl::FilterProcInfo* fpip);

		NormalizedY maximum() const;
		NormalizedY minimum() const;

		template<typename F>
		BOOL pixelwise_map(F f) {
			for (auto& elem:this->buffer)
			{
				elem = f(elem);
			}
			return true;
		}
	private:
		std::vector<NormalizedY> buffer;
	};
}

auto __stdcall GetFilterTable();

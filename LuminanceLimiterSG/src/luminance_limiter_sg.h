#pragma once

#include <aviutl/filter.hpp>

#include <algorithm>
#include <concepts>
#include <vector>


namespace luminance_limiter_sg {
	using NormalizedY = float;
	class NormalizedYBuffer {
	public:
		NormalizedYBuffer(AviUtl::FilterProcInfo* fpip) noexcept;

		NormalizedY maximum() const noexcept;
		NormalizedY minimum() const noexcept;

		template<typename F>
		BOOL pixelwise_map(F f) {
			for (auto& elem : this->buffer)
			{
				elem = f(elem);
			}
			return true;
		}

		BOOL render(AviUtl::PixelYC* target) const noexcept;
	private:
		std::vector<NormalizedY> buffer;
	};
}

auto __stdcall GetFilterTable();

/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


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
		BOOL pixelwise_map(const F&& f) {
			for (auto& elem : this->buffer)
			{
				elem = f(elem);
			}
			return true;
		}

		BOOL render(AviUtl::FilterProcInfo* fpip) const noexcept;
	private:
		std::vector<NormalizedY> buffer;
	};
}

auto __stdcall GetFilterTable();

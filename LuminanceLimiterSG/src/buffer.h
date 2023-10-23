/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once


#include <vector>

#include "aviutl/filter.hpp"

namespace luminance_limiter_sg
{
	using NormalizedY = float;
	class Buffer {
	public:
		Buffer(uint32_t width, uint32_t height) noexcept;

		const NormalizedY maximum() const noexcept;
		const NormalizedY minimum() const noexcept;

		template<typename F>
		inline const void pixelwise_map(const F&& f) {
			for (auto&& elem : buffer)
			{
				elem = f(elem);
			}
		}

		const void fetch_image(uint32_t width, uint32_t height, AviUtl::PixelYC* dst) noexcept;
		const void render(uint32_t width, uint32_t height, AviUtl::PixelYC* dst) const;
	private:
		uint32_t width = 0;
		uint32_t height = 0;
		std::vector<NormalizedY> buffer;
	};
}
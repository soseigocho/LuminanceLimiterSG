/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once


#include <vector>

#include "aviutl/filter.hpp"
#include "common_utility.h"
#include "luminance.h"

namespace luminance_limiter_sg
{
	class Buffer {
	public:
		Buffer(uint32_t width, uint32_t height) noexcept;

		const double maximum() const noexcept;
		const double minimum() const noexcept;

		template<typename F>
		inline const void pixelwise_map(const F&& f) {
			for (auto&& elem : buffer)
			{
				elem = f(elem);
			}
		}

		const void fetch_image(uint32_t width, uint32_t height, AviUtl::PixelYC* dst) noexcept;

		template<std::invocable<double, uint32_t, uint32_t> F>
		const void render(uint32_t width, uint32_t height, AviUtl::PixelYC* dst, F&& dither_f) const
		{
			AviUtl::PixelYC* row = nullptr;
			auto buf_idx = 0;
			for (auto y = 0; y < height; ++y)
			{
				row = dst + y * this->width;
				for (auto x = 0; x < width; ++x)
				{
					const auto denormalized = Luminance::denormalize_y(buffer[buf_idx]);
					using U = decltype(denormalized);
					(row + x)->y = denormalized > Luminance::y_max ? Luminance::y_max : static_cast<int16_t>(dither_f(denormalized, x, y));
					buf_idx++;
				}
			}
		};
	private:
		uint32_t width = 0;
		uint32_t height = 0;
		std::vector<double> buffer;
	};
}
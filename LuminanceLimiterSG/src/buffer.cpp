/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#include "buffer.h"

#include "luminance.h"


namespace luminance_limiter_sg
{
	Buffer::Buffer(uint32_t width, uint32_t height) noexcept
		: width(width), height(height), buffer(width * height, 0.0)
	{
	}

	const double Buffer::maximum() const noexcept
	{
		return *std::max_element(this->buffer.begin(), this->buffer.end());
	}

	const double Buffer::minimum() const noexcept
	{
		return *std::min_element(this->buffer.begin(), this->buffer.end());
	}

	const void Buffer::fetch_image(uint32_t width, uint32_t height, AviUtl::PixelYC* dst) noexcept
	{
		AviUtl::PixelYC* row = nullptr;
		auto buf_idx = 0;
		for (auto y = 0; y < height; ++y)
		{
			row = dst + y * this->width;
			for (auto x = 0; x < width; ++x)
			{
				buffer[buf_idx] = Luminance::normalize_y((row + x)->y);
				buf_idx++;
			}
		}
	}
}
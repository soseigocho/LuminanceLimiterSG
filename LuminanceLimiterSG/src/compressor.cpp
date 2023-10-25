/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#include "compressor.h"


namespace luminance_limiter_sg
{
	Compressor::Compressor(const AviUtl::FilterPlugin* const fp)
	{}

	const std::function<float(float)> Compressor::effect() const noexcept
	{
		return [](auto x) -> auto { return x; };
	}

	const void Compressor::fetch_trackbar_and_buffer(const AviUtl::FilterPlugin* const fp, const Buffer& buffer)
	{}

	const void Compressor::update_from_trackbar(const AviUtl::FilterPlugin* const fp, const uint32_t track)
	{}

	const void Compressor::used() noexcept
	{
		use = true;
	}

	const void Compressor::reset() noexcept
	{
		use = false;
	}

	const bool Compressor::is_using() const noexcept
	{
		return use;
	}
}
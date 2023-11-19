/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once

#include <functional>


namespace luminance_limiter_sg
{
	constexpr static inline auto idxy(auto elem, auto, auto) noexcept
	{
		return elem;
	}

	class DitheringFilter
	{
	public:
		std::function<double(double, uint32_t, uint32_t)> dither = idxy<double, uint32_t, uint32_t>;
	private:

	};
}
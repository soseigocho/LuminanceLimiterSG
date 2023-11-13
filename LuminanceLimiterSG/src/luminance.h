/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once


namespace luminance_limiter_sg
{
	struct Luminance
	{
		constexpr static inline auto y_max = 4096.0;
		constexpr static inline auto y_min = 0.0;
		constexpr static inline auto normalize_y(const auto y) -> auto { return static_cast<decltype(y_max)>(y) / y_max; }
		constexpr static inline auto denormalize_y(const auto y) -> auto { return y * y_max; };
	};
}
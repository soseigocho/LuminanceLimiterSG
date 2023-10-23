/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once


namespace luminance_limiter_sg
{
	class Luminance
	{
	public:
		constexpr static inline auto y_max = 4096.0f;
		constexpr static inline auto y_min = 0.0f;
	private:
	};

	template <typename T>
	T normalize_y(const T y) noexcept {
		return static_cast<float>(y) / Luminance::y_max;
	}

	template <typename T>
	T denormalize_y(const T y) noexcept {
		return static_cast<T>(y * Luminance::y_max);
	}
}
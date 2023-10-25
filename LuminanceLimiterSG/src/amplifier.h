/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once

#include <functional>

#include "common_utility.h"


namespace luminance_limiter_sg
{
	constexpr static inline auto stretch_scale(const NormalizedY peak, const NormalizedY threashold, const NormalizedY diff) noexcept;
	constexpr static inline auto stretch_diff(const NormalizedY threashold, const NormalizedY scale, const NormalizedY y) noexcept;
	const inline std::regular_invocable<NormalizedY> auto make_scale(
		const NormalizedY orig_top, const NormalizedY orig_bottom,
		const NormalizedY top_diff, const NormalizedY bottom_diff) noexcept;
	const inline std::regular_invocable<NormalizedY> auto make_gain(const NormalizedY gain) noexcept;

	class Amplifier
	{
	public:
		void update_scale(
			const NormalizedY orig_top, const NormalizedY orig_bottom,
			const NormalizedY top_diff, const NormalizedY bottom_diff) noexcept;
		void update_gain(const float gain) noexcept;

		NormalizedY scale_and_gain(const NormalizedY y) const noexcept;
	private:
		std::function<float(float)> scale = id;
		std::function<float(float)> gain = id;
	};
}
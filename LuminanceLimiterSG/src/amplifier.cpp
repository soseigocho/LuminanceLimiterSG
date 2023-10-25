/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#include "amplifier.h"


namespace luminance_limiter_sg
{
	constexpr static inline auto stretch_scale(const NormalizedY peak, const NormalizedY threashold, const NormalizedY diff) noexcept
	{
		const auto range = peak - threashold;
		const auto scaled_range = peak + diff - threashold;
		const auto scale = scaled_range / range;
		return scale;
	}

	constexpr static inline auto stretch_diff(const NormalizedY threashold, const NormalizedY scale, const NormalizedY y) noexcept
	{
		const auto stretching_range = y - threashold;
		const auto stretched_range = stretching_range * scale;
		const auto stretched = stretched_range + threashold;
		const auto diff = stretched - y;
		return diff;
	}

	const inline std::regular_invocable<float> auto make_scale(
		const NormalizedY orig_top, const NormalizedY orig_bottom,
		const NormalizedY top_diff, const NormalizedY bottom_diff) noexcept
	{
		const auto top_scale = stretch_scale(orig_top, orig_bottom, top_diff);
		const auto bottom_scale = stretch_scale(orig_bottom, orig_top, bottom_diff);

		return [=](NormalizedY y) -> NormalizedY {
			const auto upper_y_diff = stretch_diff(orig_bottom, top_scale, y);
			const auto lower_y_diff = stretch_diff(orig_top, bottom_scale, y);
			return y + upper_y_diff + lower_y_diff;
			};
	}

	const inline std::regular_invocable<float> auto make_gain(const NormalizedY gain) noexcept
	{
		return [=](NormalizedY y) {return y + gain; };
	}


	void Amplifier::update_scale(
		const NormalizedY orig_top, const NormalizedY orig_bottom,
		const NormalizedY top_diff, const NormalizedY bottom_diff) noexcept
	{
		scale = make_scale(orig_top, orig_bottom, top_diff, bottom_diff);
	}

	void Amplifier::update_gain(const NormalizedY gain) noexcept
	{
		this->gain = make_gain(gain);
	}

	NormalizedY Amplifier::scale_and_gain(NormalizedY y) const noexcept
	{
		return gain(scale(y));
	}
}
/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#include "rack_unit.h"

#include "luminance.h"
#include "project_parameter.h"


namespace luminance_limiter_sg
{
	RackUnit::RackUnit(AviUtl::FilterPlugin* fp)
	{
		const auto top_limit = normalize_y(fp->track[1]);
		const auto bottom_limit = normalize_y(fp->track[3]);
		peak_envelope_generator.set_limit(top_limit, bottom_limit);

		if (!ProjectParameter::fps())
		{
			throw std::runtime_error("Fps has not initialized.");
		}
		const auto sustain = static_cast<uint32_t>(std::floor(static_cast<float>(fp->track[6]) * ProjectParameter::fps().value() / 1000.0f));
		peak_envelope_generator.set_sustain(sustain);

		const auto release = std::ceil(static_cast<float>(fp->track[7]) * ProjectParameter::fps().value() / 1000.0f);
		peak_envelope_generator.set_release(release);
\
const auto gain_val = normalize_y(fp->track[8]);
		limiter.update_gain(gain_val);
	}

	const std::function<float(float)> RackUnit::effect() const noexcept
	{
		return [&](float y) -> float { return limiter.limit(limiter.scale_and_gain(y)); };
	}

	const void RackUnit::fetch_trackbar_and_buffer(AviUtl::FilterPlugin* fp, const Buffer& buffer)
	{
		const auto orig_top = buffer.maximum();
		const auto orig_bottom = buffer.minimum();

		const auto top_limit = normalize_y(fp->track[1]);
		const auto top_threshold_diff = normalize_y(fp->track[2]);
		const auto bottom_limit = normalize_y(fp->track[3]);
		const auto bottom_threshold_diff = normalize_y(fp->track[4]);
		const auto top_diff = normalize_y(fp->track[9]);
		const auto bottom_diff = normalize_y(fp->track[10]);

		limiter.update_scale(orig_top, orig_bottom, top_diff, bottom_diff);
		const auto gained_top = limiter.scale_and_gain(orig_top);
		const auto gained_bottom = limiter.scale_and_gain(orig_bottom);
		const auto [enveloped_top, enveloped_bottom] =
			peak_envelope_generator.update_and_get_envelope_peaks(gained_top, gained_bottom);

		const auto limit_character_interpolation_mode = static_cast<InterpolationMode>(fp->track[5]);
		limiter.update_limiter(
			top_limit, top_threshold_diff,
			bottom_limit, bottom_threshold_diff,
			enveloped_top, enveloped_bottom,
			limit_character_interpolation_mode);
	}

	const void RackUnit::used() noexcept
	{
		use = true;
	}

	const void RackUnit::reset() noexcept
	{
		use = false;
	}

	const bool RackUnit::is_using() const noexcept
	{
		return use;
	}
}
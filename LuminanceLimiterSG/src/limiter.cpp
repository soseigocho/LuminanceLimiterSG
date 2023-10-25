/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/

#include "limiter.h"

#include <algorithm>
#include <array>
#include <stdexcept>

#include "luminance.h"
#include "project_parameter.h"

namespace luminance_limiter_sg {
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

	const inline std::function<
		std::function<float(float)>(
			NormalizedY, NormalizedY,
			NormalizedY, NormalizedY,
			NormalizedY, NormalizedY)> select_character(InterpolationMode mode)
	{
		switch (mode)
		{
		case InterpolationMode::Linear:
			return make_linear_character;
		case InterpolationMode::Lagrange:
			return make_lagrange_character;
		case InterpolationMode::Spline:
			return make_spline_character;
		default:
			throw std::runtime_error("Error: Illegal interpolation mode.");
		}
	}

	Limiter::Limiter(const AviUtl::FilterPlugin* const fp)
	{
		const auto top_limit = normalize_y(fp->track[11]);
		const auto bottom_limit = normalize_y(fp->track[12]);
		peak_envelope_generator.set_limit(top_limit, bottom_limit);

		if (!ProjectParameter::fps())
		{
			throw std::runtime_error("Fps has not initialized.");
		}
		const auto sustain = static_cast<uint32_t>(std::floor(static_cast<float>(fp->track[7]) * ProjectParameter::fps().value() / 1000.0f));
		peak_envelope_generator.set_sustain(sustain);

		const auto release = std::ceil(static_cast<float>(fp->track[8]) * ProjectParameter::fps().value() / 1000.0f);
		peak_envelope_generator.set_release(release);
		const auto gain_val = normalize_y(fp->track[3]);
		update_gain(gain_val);
	}

	const std::function<float(float)> Limiter::effect() const noexcept
	{
		return [&](float y) -> float { return limit(scale_and_gain(y)); };
	}

	const void Limiter::fetch_trackbar_and_buffer(const AviUtl::FilterPlugin* const fp, const Buffer& buffer)
	{
		const auto orig_top = buffer.maximum();
		const auto orig_bottom = buffer.minimum();

		const auto top_limit = normalize_y(fp->track[11]);
		const auto bottom_limit = normalize_y(fp->track[12]);
		
		auto thresholds = std::array<uint32_t, 2>({
			static_cast<uint32_t>(fp->track[4]), static_cast<uint32_t>(fp->track[5])});
		std::sort(thresholds.begin(), thresholds.end());

		const auto top_threshold_diff = thresholds[1] - top_limit;
		const auto bottom_threshold_diff = thresholds[2] - bottom_limit;
		const auto top_diff = normalize_y(fp->track[1]);
		const auto bottom_diff = normalize_y(fp->track[2]);
		update_scale(orig_top, orig_bottom, top_diff, bottom_diff);

		const auto gained_top = scale_and_gain(orig_top);
		const auto gained_bottom = scale_and_gain(orig_bottom);
		const auto [enveloped_top, enveloped_bottom] =
			peak_envelope_generator.update_and_get_envelope_peaks(gained_top, gained_bottom);

		const auto limit_character_interpolation_mode = static_cast<InterpolationMode>(fp->track[9]);
		update_limiter(
			top_limit, top_threshold_diff,
			bottom_limit, bottom_threshold_diff,
			enveloped_top, enveloped_bottom,
			limit_character_interpolation_mode);
	}

	const void Limiter::update_from_trackbar(const AviUtl::FilterPlugin* const fp, const uint32_t track) noexcept
	{
		switch (track)
		{
		case 4U:
		{
			const auto gain_val = normalize_y(fp->track[3]);
			update_gain(gain_val);
			break;
		}
		case 8U:
		{
			const auto sustain = static_cast<uint32_t>(std::floor(static_cast<float>(fp->track[7]) * ProjectParameter::fps().value() / 1000.0f));
			peak_envelope_generator.set_sustain(sustain);
			break;
		}
		case 9U:
		{
			const auto release = std::ceil(static_cast<float>(fp->track[8]) * ProjectParameter::fps().value() / 1000.0f);
			peak_envelope_generator.set_release(release);
			break;
		}
		case 12U:
		{			
			const auto top_limit = normalize_y(fp->track[11]);
			const auto bottom_limit = normalize_y(fp->track[12]);
			peak_envelope_generator.set_limit(top_limit, bottom_limit);
			break;
		}
		case 13U:
		{
			const auto top_limit = normalize_y(fp->track[11]);
			const auto bottom_limit = normalize_y(fp->track[12]);
			peak_envelope_generator.set_limit(top_limit, bottom_limit);
			break;
		}
		default:
			break;
		}
	}

	const void Limiter::used() noexcept
	{
		use = true;
	}

	const void Limiter::reset() noexcept
	{
		use = false;
	}

	const bool Limiter::is_using() const noexcept
	{
		return use;
	}

	BOOL Limiter::update_scale(
		const NormalizedY orig_top, const NormalizedY orig_bottom,
		const NormalizedY top_diff, const NormalizedY bottom_diff)
	{
		this->scale = make_scale(orig_top, orig_bottom, top_diff, bottom_diff);
		return true;
	}

	BOOL Limiter::update_gain(const NormalizedY gain)
	{
		this->gain = make_gain(gain);
		return true;
	}

	BOOL Limiter::update_limiter(
		const NormalizedY top_limit, const NormalizedY top_threshold_diff,
		const NormalizedY bottom_limit, const NormalizedY bottom_threshold_diff,
		const NormalizedY top_peak, const NormalizedY bottom_peak,
		InterpolationMode mode)
	{
		this->limiter = make_limit(
			make_character(
				top_limit, top_threshold_diff,
				bottom_limit, bottom_threshold_diff,
				top_peak, bottom_peak,
				select_character(mode)),
			top_limit, bottom_limit);

		return true;
	}

	NormalizedY Limiter::scale_and_gain(const NormalizedY y) const
	{
		return this->gain(this->scale(y));
	}

	NormalizedY Limiter::limit(const NormalizedY y) const
	{
		return this->limiter(y);
	}
}
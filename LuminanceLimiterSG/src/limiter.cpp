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

#include "common_utility.h"
#include "project_parameter.h"

namespace luminance_limiter_sg
{
	const inline std::function<
		std::function<double(double)>(
			double, double,
			double, double,
			double, double)> select_character(InterpolationMode mode)
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
		const auto top_limit = Luminance::normalize_y(fp->track[1]);
		const auto bottom_limit = Luminance::normalize_y(fp->track[4]);
		peak_envelope_generator.set_limit(top_limit, bottom_limit);

		if (!ProjectParameter::get_fps())
		{
			throw std::runtime_error("Fps has not initialized.");
		}
		const auto sustain = static_cast<uint32_t>(std::floor(static_cast<double>(fp->track[5]) * ProjectParameter::get_fps().value() / 1000.0));
		peak_envelope_generator.set_sustain(sustain);

		const auto release = std::ceil(static_cast<double>(fp->track[6]) * ProjectParameter::get_fps().value() / 1000.0);
		peak_envelope_generator.set_release(release);
	}

	const std::function<double(double)> Limiter::effect() const noexcept
	{
		return [&](double y) -> double { return limit(y); };
	}

	const void Limiter::fetch_trackbar_and_buffer(const AviUtl::FilterPlugin* const fp, const Buffer& buffer)
	{
		const auto orig_top = buffer.maximum();
		const auto orig_bottom = buffer.minimum();

		const auto top_limit = Luminance::normalize_y(fp->track[1]);
		const auto bottom_limit =Luminance::normalize_y(fp->track[4]);
		
		auto thresholds = std::array<double, 2>({ Luminance::normalize_y(fp->track[2]), Luminance::normalize_y(fp->track[3]) });
		std::sort(thresholds.begin(), thresholds.end());

		const auto [enveloped_top, enveloped_bottom] =
			peak_envelope_generator.update_and_get_envelope_peaks(orig_top, orig_bottom);

		const auto limit_character_interpolation_mode = static_cast<InterpolationMode>(fp->track[7]);
		update_limiter(
			top_limit, thresholds[1],
			bottom_limit, thresholds[0],
			enveloped_top, enveloped_bottom,
			limit_character_interpolation_mode);
	}

	const void Limiter::update_from_trackbar(const AviUtl::FilterPlugin* const fp, const uint32_t track) noexcept
	{
		switch (track)
		{
		case 8U:
		{
			const auto sustain = static_cast<uint32_t>(std::floor(static_cast<double>(fp->track[5]) * ProjectParameter::get_fps().value() / 1000.0));
			peak_envelope_generator.set_sustain(sustain);
			break;
		}
		case 9U:
		{
			const auto release = std::ceil(static_cast<double>(fp->track[6]) * ProjectParameter::get_fps().value() / 1000.0);
			peak_envelope_generator.set_release(release);
			break;
		}
		case 13U:
		{			
			const auto top_limit = Luminance::normalize_y(fp->track[1]);
			const auto bottom_limit = Luminance::normalize_y(fp->track[4]);
			peak_envelope_generator.set_limit(top_limit, bottom_limit);
			break;
		}
		case 14U:
		{
			const auto top_limit = Luminance::normalize_y(fp->track[1]);
			const auto bottom_limit = Luminance::normalize_y(fp->track[4]);
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

	BOOL Limiter::update_limiter(
		const double top_limit, const double top_threshold,
		const double bottom_limit, const double bottom_threshold,
		const double top_peak, const double bottom_peak,
		InterpolationMode mode)
	{
		this->limiter = make_limit(
			make_character(
				top_limit, top_threshold,
				bottom_limit, bottom_threshold,
				top_peak, bottom_peak,
				select_character(mode)),
			top_limit, bottom_limit);

		return true;
	}

	double Limiter::limit(const double y) const
	{
		return this->limiter(y);
	}
}
/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once

#include "luminance_limiter_sg.h"

#include <algorithm>
#include <functional>

#include "buffer.h"
#include "interpolation.h"
#include "luminance.h"
#include "peak_envelope_generator.h"


namespace luminance_limiter_sg {
	template<typename F>
	constexpr static inline auto make_some_charactors(
		const double top_limit, const double top_threshold,
		const double bottom_limit, const double bottom_threshold,
		const double top_peak, const double bottom_peak,
		const F&& interp_method)
	{
		const auto x0 = bottom_peak <= bottom_limit ? bottom_peak : bottom_limit;
		const auto x3 = top_peak >= top_limit ? top_peak : top_limit;
		auto xs = std::vector{ x0, bottom_threshold, top_threshold, x3};
		std::sort(xs.begin(), xs.end());
		auto ys = std::vector{ bottom_limit, bottom_threshold, top_threshold, top_limit };
		std::sort(ys.begin(), ys.end());
		return interp_method(std::move(xs), std::move(ys));
	}

	constexpr static inline auto make_linear_character = [](
		const double top_limit, const double top_threshold,
		const double bottom_limit, const double bottom_threshold,
		const double top_peak, const double bottom_peak)
		{
			return make_some_charactors(
				top_limit, top_threshold,
				bottom_limit, bottom_threshold,
				top_peak, bottom_peak,
				linear_interp);
		};

	constexpr static inline auto make_lagrange_character = [](
		const double top_limit, const double top_threshold,
		const double bottom_limit, const double bottom_threshold,
		const double top_peak, const double bottom_peak)
		{
			return make_some_charactors(
				top_limit, top_threshold,
				bottom_limit, bottom_threshold,
				top_peak, bottom_peak,
				lagrange_interp);
		};

	constexpr static inline auto make_spline_character = [](
		const double top_limit, const double top_threshold,
		const double bottom_limit, const double bottom_threshold,
		const double top_peak, const double bottom_peak)
		{
			return make_some_charactors(
				top_limit, top_threshold,
				bottom_limit, bottom_threshold,
				top_peak, bottom_peak,
				spline_interp);
		};

	const static inline std::function<
		std::function<double(double)>(
			double, double,
			double, double,
			double, double)> select_character(InterpolationMode mode);

	template<typename F>
	const static inline std::function<double(double)> make_character(
		const double top_limit, const double top_threshold,
		const double bottom_limit, const double bottom_threshold,
		const double top_peak, const double bottom_peak,
		const F&& character)
	{
		return character(top_limit, top_threshold, bottom_limit, bottom_threshold, top_peak, bottom_peak);
	}

	template<typename F>
	constexpr static inline auto make_limit(
		const F&& character,
		const double top_limit,
		const double bottom_limit)
	{
		return [=, character = std::move(character)](const double y) -> double {
			const auto charactered = character(y);
			if (charactered > top_limit)
			{
				return top_limit;
			}
			else if (charactered < bottom_limit)
			{
				return bottom_limit;
			}
			else
			{
				return charactered;
			}
		};
	}

	class Limiter
	{
	public:
		Limiter(const AviUtl::FilterPlugin* const fp);

		const std::function<double(double)> effect() const noexcept;
		const void fetch_trackbar_and_buffer(const AviUtl::FilterPlugin* const fp, const Buffer& buffer);
		const void update_from_trackbar(const AviUtl::FilterPlugin* const fp, const uint32_t track) noexcept;

		const void used() noexcept ;
		const void reset() noexcept ;

		const bool is_using() const noexcept ;
	private:
		bool use = false;

		PeakEnvelopeGenerator peak_envelope_generator;

		std::function<double(double)> limiter = id;

		BOOL update_limiter(
			const double top_limit, const double top_threshold,
			const double bottom_limit, const double bottom_threshold,
			const double top_peak, const double bottom_peak,
			InterpolationMode mode);
		double limit(const double y) const;

	};
}
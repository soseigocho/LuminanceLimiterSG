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
#include <optional>
#include <stdexcept>

#include "buffer.h"
#include "peak_envelope_generator.h"
#include "rack_unit.h"


namespace luminance_limiter_sg {
	constexpr static inline auto stretch_scale(const NormalizedY peak, const NormalizedY threashold, const NormalizedY diff) noexcept;
	constexpr static inline auto stretch_diff(const NormalizedY threashold, const NormalizedY scale, const NormalizedY y) noexcept;
	const inline std::function<NormalizedY(NormalizedY)> make_scale(
		const NormalizedY orig_top, const NormalizedY orig_bottom,
		const NormalizedY top_diff, const NormalizedY bottom_diff) noexcept;
	const inline std::function<NormalizedY(NormalizedY)> make_gain(const NormalizedY gain) noexcept;

	enum class InterpolationMode : int32_t
	{
		Linear,
		Lagrange,
		Spline
	};

	const inline std::function<float(float)> linear_interp(const std::vector<float>&& xs, const std::vector<float>&& ys);
	const inline std::function<float(float)> lagrange_interp(const std::vector<float>&& xs, const std::vector<float>&& ys);
	constexpr static inline auto tdma(const std::vector<float>& a, const std::vector<float>& b, const std::vector<float>& c, const std::vector<float>& d);
	constexpr static inline std::optional<int> inner_binary_search(const std::vector<float>& xs, const float x, const int max_idx, const int min_idx);
	constexpr static inline auto binary_search(const std::vector<float>& xs, const float x);
	const inline std::function<float(float)> spline_interp(const std::vector<float>&& xs, const std::vector<float>&& ys);

	template<typename F>
	constexpr static inline auto make_some_charactors(
		const NormalizedY top_limit, const NormalizedY top_threshold_diff,
		const NormalizedY bottom_limit, const NormalizedY bottom_threshold_diff,
		const NormalizedY top_peak, const NormalizedY bottom_peak,
		const F&& interp_method)
	{
		const auto top_threshold = top_limit + top_threshold_diff;
		const auto bottom_threshold = bottom_limit + bottom_threshold_diff;
		const auto x0 = bottom_peak <= bottom_limit ? bottom_peak : bottom_limit;
		const auto x3 = top_peak >= top_limit ? top_peak : top_limit;
		auto xs = std::vector{ x0, bottom_threshold, top_threshold, x3 };
		std::sort(xs.begin(), xs.end());
		auto ys = std::vector{ bottom_limit, bottom_threshold, top_threshold, top_limit };
		std::sort(ys.begin(), ys.end());
		return interp_method(std::move(xs), std::move(ys));
	}

	constexpr static inline auto make_linear_character = [](
		const NormalizedY top_limit, const NormalizedY top_threshold_diff,
		const NormalizedY bottom_limit, const NormalizedY bottom_threshold_diff,
		const NormalizedY top_peak, const NormalizedY bottom_peak)
		{
			return make_some_charactors(
				top_limit, top_threshold_diff,
				bottom_limit, bottom_threshold_diff,
				top_peak, bottom_peak,
				linear_interp);
		};

	constexpr static inline auto make_lagrange_character = [](
		const NormalizedY top_limit, const NormalizedY top_threshold_diff,
		const NormalizedY bottom_limit, const NormalizedY bottom_threshold_diff,
		const NormalizedY top_peak, const NormalizedY bottom_peak)
		{
			return make_some_charactors(
				top_limit, top_threshold_diff,
				bottom_limit, bottom_threshold_diff,
				top_peak, bottom_peak,
				lagrange_interp);
		};

	constexpr static inline auto make_spline_character = [](
		const NormalizedY top_limit, const NormalizedY top_threshold_diff,
		const NormalizedY bottom_limit, const NormalizedY bottom_threshold_diff,
		const NormalizedY top_peak, const NormalizedY bottom_peak)
		{
			return make_some_charactors(
				top_limit, top_threshold_diff,
				bottom_limit, bottom_threshold_diff,
				top_peak, bottom_peak,
				spline_interp);
		};

	const static inline std::function<
		std::function<float(float)>(
			NormalizedY, NormalizedY,
			NormalizedY, NormalizedY,
			NormalizedY, NormalizedY)> select_character(InterpolationMode mode);

	template<typename F>
	const static inline std::function<float(float)> make_character(
		const NormalizedY top_limit, const NormalizedY top_threshold_diff,
		const NormalizedY bottom_limit, const NormalizedY bottom_threshold_diff,
		const NormalizedY top_peak, const NormalizedY bottom_peak,
		const F&& character)
	{
		return character(top_limit, top_threshold_diff, bottom_limit, bottom_threshold_diff, top_peak, bottom_peak);
	}

	template<typename F>
	constexpr static inline auto make_limit(
		const F&& character,
		const NormalizedY top_limit,
		const NormalizedY bottom_limit)
	{
		return [=, character = std::move(character)](const NormalizedY y) -> NormalizedY {
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

	constexpr inline auto id = [](auto x) -> auto { return x; };

	class Limiter final : public IRackUnit
	{
	public:
		Limiter(const AviUtl::FilterPlugin* const fp);

		const std::function<float(float)> effect() const noexcept;
		const void fetch_trackbar_and_buffer(const AviUtl::FilterPlugin* const fp, const Buffer& buffer) override;
		const void update_from_trackbar(const AviUtl::FilterPlugin* const fp, const uint32_t track) noexcept override;

		const void used() noexcept override;
		const void reset() noexcept override;

		const bool is_using() const noexcept override;
	private:
		bool use = false;
		PeakEnvelopeGenerator peak_envelope_generator;

		std::function<float(float)> scale = id;
		std::function<float(float)> gain = id;
		std::function<NormalizedY(NormalizedY)> limiter = id;

		BOOL update_scale(
			const NormalizedY orig_top, const NormalizedY orig_bottom,
			const NormalizedY top_diff, const NormalizedY bottom_diff);
		BOOL update_gain(const NormalizedY gain);
		BOOL update_limiter(
			const NormalizedY top_limit, const NormalizedY top_threshold_diff,
			const NormalizedY bottom_limit, const NormalizedY bottom_threshold_diff,
			const NormalizedY top_peak, const NormalizedY bottom_peak,
			InterpolationMode mode);
		NormalizedY scale_and_gain(const NormalizedY y) const;
		NormalizedY limit(const NormalizedY y) const;

	};
}
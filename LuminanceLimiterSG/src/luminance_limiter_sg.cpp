/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#include "luminance_limiter_sg.h"

#include <array>
#include <deque>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>


namespace luminance_limiter_sg {
	constexpr static inline auto name = "LuminanceLimiterSG";
	constexpr static inline auto track_n = 10u;
	constexpr static inline auto track_name =
		std::array<const char*, track_n>{ "è„å¿", "è„å¿Ëáíl", "â∫å¿", "â∫å¿Ëáíl", "ï‚ä‘”∞ƒﬁ", "éùë±", "ó]âC", "µÃæØƒ", "ñæÇÈÇ≥", "à√Ç≥"};
	constexpr static inline auto track_default = std::array<int32_t, track_n>{ 4096, -1024, 256, 512, 0, 4, 8, -128, 512, -64 };
	constexpr static inline auto track_s = std::array<int32_t, track_n>{ 0, -4096, 0, 0, 0, 0, 0, -4096, -4096, -4096 };
	constexpr static inline auto track_e = std::array<int32_t, track_n>{ 4096, 0, 4096, 4096, 1, 256, 256, 4096, 4096, 4096 };
	constexpr static inline auto information = "LuminanceLimiterSG v0.2.0 by ëeêªåﬁí∑";

	constexpr static inline auto y_max = 4096.0f;
	constexpr static inline auto y_min = 0.0f;

	template <typename T>
	NormalizedY normalize_y(const T y) noexcept {
		return static_cast<float>(y) / y_max;
	}

	template <typename T>
	T denormalize_y(const NormalizedY normalized_y) noexcept {
		return static_cast<T>(normalized_y * y_max);
	}

	NormalizedYBuffer::NormalizedYBuffer(AviUtl::FilterProcInfo* fpip) noexcept
	{
		const auto max_width = fpip->max_w;
		const auto max_height = fpip->max_h;
		this->buffer = std::vector(max_width * max_height, 0.0f);

		const auto width = fpip->w;
		const auto height = fpip->h;

		AviUtl::PixelYC* row = nullptr;
		auto buf_idx = 0;
		for (auto y = 0; y < height; ++y)
		{
			row = static_cast<AviUtl::PixelYC*>(fpip->ycp_edit) + y * max_width;
			for (auto x = 0; x < width; ++x)
			{
				this->buffer[buf_idx] = normalize_y((row + x)->y);
				buf_idx++;
			}
		}
	}

	NormalizedY NormalizedYBuffer::maximum() const noexcept
	{
		return *std::max_element(this->buffer.begin(), this->buffer.end());
	}

	NormalizedY NormalizedYBuffer::minimum() const noexcept
	{
		return *std::min_element(this->buffer.begin(), this->buffer.end());
	}

	BOOL NormalizedYBuffer::render(AviUtl::FilterProcInfo* fpip) const noexcept
	{
		const auto max_width = fpip->max_w;
		const auto width = fpip->w;
		const auto height = fpip->h;

		AviUtl::PixelYC* row = nullptr;
		auto buf_idx = 0;
		for (auto y = 0; y < height; ++y)
		{
			row = static_cast<AviUtl::PixelYC*>(fpip->ycp_edit) + y * max_width;
			for (auto x = 0; x < width; ++x)
			{
				(row + x)->y = denormalize_y<int16_t>(this->buffer[buf_idx]);
				buf_idx++;
			}
		}
		return true;
	}

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

	constexpr static inline auto make_scale(
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

	constexpr static inline auto make_gain(const NormalizedY gain) noexcept
	{
		return [=](NormalizedY y) {return y + gain; };
	}

	enum class InterporationMode
	{
		Linear,
		Lagrange,
	};

	constexpr static inline auto lienar_interp(const std::vector<float>&& xs, const std::vector<float>&& ys)
	{
		if (xs.size() != ys.size())
		{
			throw std::runtime_error("Error: xs and ys must have the same size.");
		}

		if (xs.size() < 2)
		{
			throw std::runtime_error("Error: xs and ys must have at least 2 elements.");
		}

		auto n = xs.size();
		auto t = [=](float x, size_t i) {
			if (x < xs[i] || x > xs[i + 1])
			{
				throw std::range_error("Error: x is out of range.");
			}
			return (x - xs[i]) / (xs[i + 1] - xs[i]);
		};

		std::vector<float> slopes(n - 1);
		for (auto i = 0; i < n - 1; ++i)
		{
			slopes[i] = (ys[i + 1] - ys[i]) / (xs[i + 1] - xs[i]);
		}

		return [=](float x) -> float {
			if (x < xs[0] || x > xs[n - 1])
			{
				throw std::range_error("Error: x is out of range.");
			}

			auto left = 0;
			auto right = n - 1;
			while (left + 1 < right)
			{
				const auto mid = (left + right) / 2;
				if (xs[mid] <= x)
				{
					left = mid;
				}
				else
				{
					right = mid;
				}
			}

			const auto idx = left;
			return ys[idx] + slopes[idx] * (x - xs[idx]) * t(x, idx);
		};
	}

	constexpr static inline auto lagrange_interp(const std::vector<float>&& xs, const std::vector<float>&& ys)
	{
		if (xs.size() != ys.size())
		{
			throw std::runtime_error("Error: xs and ys must have the same size.");
		}

		if (xs.size() < 2)
		{
			throw std::runtime_error("Error: xs and ys must have at least 2 elements.");
		}

		return [=](float x) {
			auto sum = 0.0f;
			for (auto i = 0; i < xs.size(); ++i)
			{
				auto prod = 1.0f;
				for (auto j = 0; j < xs.size(); ++j)
				{
					if (i != j)
					{
						prod *= (x - xs[j]) / (xs[i] - xs[j]);
					}
				}
				sum += ys[i] * prod;
			}
			return sum;
		};
	}

	constexpr static inline auto make_linear_character(
		const NormalizedY top_limit, const NormalizedY top_threshold_diff,
		const NormalizedY bottom_limit, const NormalizedY bottom_threshold_diff,
		const NormalizedY top_peak, const NormalizedY bottom_peak)
	{
		const auto top_threshold = top_limit + top_threshold_diff;
		const auto bottom_threshold = bottom_limit + bottom_threshold_diff;
		const auto x0 = bottom_peak <= bottom_limit ? bottom_peak : bottom_limit;
		const auto x3 = top_peak >= top_limit ? top_peak : top_limit;
		auto xs = std::vector{ x0, bottom_threshold, top_threshold, x3 };
		std::sort(xs.begin(), xs.end());
		auto ys = std::vector{ bottom_limit, bottom_threshold, top_threshold, top_limit };
		std::sort(ys.begin(), ys.end());
		return lienar_interp(std::move(xs), std::move(ys));
	}

	constexpr static inline auto make_lagrange_character(
		const NormalizedY top_limit, const NormalizedY top_threshold_diff,
		const NormalizedY bottom_limit, const NormalizedY bottom_threshold_diff,
		const NormalizedY top_peak, const NormalizedY bottom_peak)
	{
		const auto top_threshold = top_limit + top_threshold_diff;
		const auto bottom_threshold = bottom_limit + bottom_threshold_diff;
		const auto x0 = bottom_peak <= bottom_limit ? bottom_peak : bottom_limit;
		const auto x3 = top_peak >= top_limit ? top_peak : top_limit;
		auto xs = std::vector{ x0, bottom_threshold, top_threshold, x3 };
		std::sort(xs.begin(), xs.end());
		auto ys = std::vector{ bottom_limit, bottom_threshold, top_threshold, top_limit };
		std::sort(ys.begin(), ys.end());
		return lagrange_interp(std::move(xs), std::move(ys));
	}

	const static inline std::function<float(float)> make_character(
		const InterporationMode mode,
		const NormalizedY top_limit, const NormalizedY top_threshold_diff,
		const NormalizedY bottom_limit, const NormalizedY bottom_threshold_diff,
		const NormalizedY top_peak, const NormalizedY bottom_peak)
	{
		switch (mode)
		{
		case InterporationMode::Linear:
			return make_linear_character(top_limit, top_threshold_diff, bottom_limit, bottom_threshold_diff, top_peak, bottom_peak);
		case InterporationMode::Lagrange:
			return make_lagrange_character(top_limit, top_threshold_diff, bottom_limit, bottom_threshold_diff, top_peak, bottom_peak);
		default:
			throw std::runtime_error("Error: Illegal interpolation mode.");
		}
	}

	template <typename F>
	constexpr static inline auto make_limit(
		const F& character,
		const NormalizedY top_limit,
		const NormalizedY bottom_limit)
	{
		return [=, &character](const NormalizedY y) -> NormalizedY {
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

	class PeakEnvelopeGenerator {
	public:
		BOOL set_limit(const NormalizedY top, const NormalizedY bottom) noexcept {
			top_limit = top;
			bottom_limit = bottom;
			return true;
		}

		BOOL set_sustain(const uint32_t sustain) {
			this->sustain = sustain;

			if (sustain == 0)
			{
				this->top_peak_lifetime_buffer = std::nullopt;
				this->bottom_peak_lifetime_buffer = std::nullopt;
				this->active_top_peak_buffer = std::nullopt;
				this->active_bottom_peak_buffer = std::nullopt;
				return false;
			}

			if (!top_peak_lifetime_buffer.has_value() || !active_top_peak_buffer.has_value()
				|| !bottom_peak_lifetime_buffer.has_value() || !active_bottom_peak_buffer.has_value())
			{
				this->top_peak_lifetime_buffer =
					std::make_optional<std::deque<NormalizedY>>(sustain + 1, -std::numeric_limits<NormalizedY>::infinity());
				this->active_top_peak_buffer = std::make_optional<std::deque<NormalizedY>>();
				this->bottom_peak_lifetime_buffer =
					std::make_optional<std::deque<NormalizedY>>(sustain + 1, std::numeric_limits<NormalizedY>::infinity());
				this->active_bottom_peak_buffer = std::make_optional<std::deque<NormalizedY>>();

				return true;
			}

			if (top_peak_lifetime_buffer.value().size() == (sustain + 1)
				&& bottom_peak_lifetime_buffer.value().size() == (sustain + 1))
			{
				return true;
			}

			if (top_peak_lifetime_buffer.value().size() > (sustain + 1))
			{
				top_peak_lifetime_buffer.value().erase(
					top_peak_lifetime_buffer.value().begin(),
					top_peak_lifetime_buffer.value().begin()
					+ (top_peak_lifetime_buffer.value().size() - (sustain + 1)));
			}

			if (bottom_peak_lifetime_buffer.value().size() > (sustain + 1))
			{
				bottom_peak_lifetime_buffer.value().erase(
					bottom_peak_lifetime_buffer.value().begin(),
					bottom_peak_lifetime_buffer.value().begin()
					+ (bottom_peak_lifetime_buffer.value().size() - (sustain + 1)));
			}

			if (top_peak_lifetime_buffer.value().size() < (sustain + 1))
			{
				const auto top_shrink_size = sustain + 1 - top_peak_lifetime_buffer.value().size();
				for (auto i = 0; i < top_shrink_size; i++)
				{
					top_peak_lifetime_buffer.value().push_front(0.0f);
				}
			}

			if (bottom_peak_lifetime_buffer.value().size() < (sustain + 1))
			{
				const auto bottom_shrink_size = sustain + 1 - bottom_peak_lifetime_buffer.value().size();
				for (auto i = 0; i < bottom_shrink_size; i++)
				{
					bottom_peak_lifetime_buffer.value().push_front(0.0f);
				}
			}

			return true;
		}

		BOOL set_release(const uint32_t release)
		{
			this->release = release;
			return true;
		}

		std::array<NormalizedY, 2> hold_peaks(const NormalizedY top_peak, const NormalizedY bottom_peak) noexcept {
			if (!top_peak_lifetime_buffer.has_value() || !active_top_peak_buffer.has_value()
				|| !bottom_peak_lifetime_buffer.has_value() || !active_bottom_peak_buffer.has_value())
			{
				return { top_peak, bottom_peak };
			}

			while (!active_top_peak_buffer.value().empty()
				&& active_top_peak_buffer.value().back() < top_peak)
			{
				active_top_peak_buffer.value().pop_back();
			}

			while (!active_bottom_peak_buffer.value().empty()
				&& active_bottom_peak_buffer.value().back() > bottom_peak)
			{
				active_bottom_peak_buffer.value().pop_back();
			}

			active_top_peak_buffer.value().push_back(top_peak);
			active_bottom_peak_buffer.value().push_back(bottom_peak);

			top_peak_lifetime_buffer.value().push_back(top_peak);
			const auto lifetimed_top_peak = top_peak_lifetime_buffer.value().front();
			top_peak_lifetime_buffer.value().pop_front();
			bottom_peak_lifetime_buffer.value().push_back(bottom_peak);
			const auto lifetimed_bottom_peak = bottom_peak_lifetime_buffer.value().front();
			bottom_peak_lifetime_buffer.value().pop_front();

			if (active_top_peak_buffer.value().front() == lifetimed_top_peak)
			{
				active_top_peak_buffer.value().pop_front();
			}
			const auto current_top_peak = active_top_peak_buffer.value().empty() ? top_peak : active_top_peak_buffer.value().front();

			if (active_bottom_peak_buffer.value().front() == lifetimed_bottom_peak)
			{
				active_bottom_peak_buffer.value().pop_front();
			}
			const auto current_bottom_peak = active_bottom_peak_buffer.value().empty() ? bottom_peak : active_bottom_peak_buffer.value().front();

			return { current_top_peak, current_bottom_peak };
		}

		std::array<NormalizedY, 2> wrap_peaks(const NormalizedY top_peak, const NormalizedY bottom_peak) noexcept {
			NormalizedY wraped_top;
			if (ongoing_top_peak == top_peak)
			{
				wraped_top = top_peak;
				top_peak_duration = 0u;
			}
			else
			{
				top_peak_duration++;
				const auto coefficient = -(ongoing_top_peak - bottom_limit) / static_cast<float>(release);
				const auto slice = ongoing_top_peak;
				const auto released = coefficient * static_cast<float>(top_peak_duration) + slice;
				if (top_peak >= released)
				{
					wraped_top = top_peak;
					ongoing_top_peak = top_peak;
					top_peak_duration = 0u;
				}
				else
				{
					wraped_top = released;
				}
			}

			NormalizedY wraped_bottom;
			if (ongoing_bottom_peak == bottom_peak)
			{
				wraped_bottom = bottom_peak;
				bottom_peak_duration = 0u;
			}
			else
			{
				bottom_peak_duration++;
				const auto coefficient = -(ongoing_bottom_peak - top_limit) / static_cast<float>(release);
				const auto slice = ongoing_bottom_peak;
				const auto released = coefficient * static_cast<float>(bottom_peak_duration) + slice;
				if (bottom_peak <= released)
				{
					wraped_bottom = bottom_peak;
					ongoing_bottom_peak = bottom_peak;
					bottom_peak_duration = 0u;
				}
				else
				{
					wraped_bottom = released;
				}
			}
			return { wraped_top, wraped_bottom };
		}

		std::array<NormalizedY, 2> update_and_get_envelope_peaks(const NormalizedY top_peak, const NormalizedY bottom_peak) noexcept {
			const auto [current_top_peak, current_bottom_peak] = hold_peaks(top_peak, bottom_peak);
			const auto [wrapped_top_peak, wrapped_bottom_peak] = wrap_peaks(current_top_peak, current_bottom_peak);
			return { wrapped_top_peak, wrapped_bottom_peak };
		};

	private:
		std::optional<std::deque<NormalizedY>> top_peak_lifetime_buffer = std::nullopt;
		std::optional<std::deque<NormalizedY>> bottom_peak_lifetime_buffer = std::nullopt;
		std::optional<std::deque<NormalizedY>> active_top_peak_buffer = std::nullopt;
		std::optional<std::deque<NormalizedY>> active_bottom_peak_buffer = std::nullopt;
		uint32_t sustain = 0u;
		uint32_t release = 0u;
		NormalizedY ongoing_top_peak = 0.0f;
		uint32_t top_peak_duration = 0u;
		NormalizedY ongoing_bottom_peak = 0.0f;
		uint32_t bottom_peak_duration = 0u;

		NormalizedY top_limit = 0.0f;
		NormalizedY bottom_limit = 0.0f;
	};

	static auto peak_envelope_generator = PeakEnvelopeGenerator();

	static inline BOOL func_proc(AviUtl::FilterPlugin* fp, AviUtl::FilterProcInfo* fpip)
	{
		auto normalized_y_buffer = NormalizedYBuffer(fpip);

		const auto orig_top = normalized_y_buffer.maximum();
		const auto orig_bottom = normalized_y_buffer.minimum();
		const auto top_diff = normalize_y(fp->track[8]);
		const auto bottom_diff = normalize_y(fp->track[9]);
		const auto scale = make_scale(orig_top, orig_bottom, top_diff, bottom_diff);

		const auto gain_val = normalize_y(fp->track[7]);
		const auto gain = make_gain(gain_val);

		const auto scale_and_gain = [&](NormalizedY y) {return gain(scale(y)); };

		const auto top_limit = normalize_y(fp->track[0]);
		const auto top_threshold_diff = normalize_y(fp->track[1]);
		const auto bottom_limit = normalize_y(fp->track[2]);
		const auto bottom_threshold_diff = normalize_y(fp->track[3]);
		peak_envelope_generator.set_limit(top_limit, bottom_limit);

		const auto sustain = static_cast<uint32_t>(fp->track[5]);
		peak_envelope_generator.set_sustain(sustain);
		const auto release = static_cast<uint32_t>(fp->track[6]);
		peak_envelope_generator.set_release(release);

		const auto gained_top = scale_and_gain(orig_top);
		const auto gained_bottom = scale_and_gain(orig_bottom);
		const auto [enveloped_top, enveloped_bottom] =
			peak_envelope_generator.update_and_get_envelope_peaks(gained_top, gained_bottom);

		const auto limit_character_interpolation_mode = static_cast<InterporationMode>(fp->track[4]);
		const auto character =
			make_character(limit_character_interpolation_mode,
				top_limit, top_threshold_diff,
				bottom_limit, bottom_threshold_diff,
				gained_top, gained_bottom);

		const auto limit = make_limit(character, top_limit, bottom_limit);

		normalized_y_buffer.pixelwise_map([&](NormalizedY y) -> NormalizedY {return limit(scale_and_gain(y)); });

		normalized_y_buffer.render(fpip);

		return true;
	}
}


constexpr AviUtl::FilterPluginDLL filter{
	.flag = AviUtl::FilterPlugin::Flag::ExInformation,
	.name = luminance_limiter_sg::name,
	.track_n = luminance_limiter_sg::track_n,
	.track_name = const_cast<const char**>(std::data(luminance_limiter_sg::track_name)),
	.track_default = const_cast<int32_t*>(std::data(luminance_limiter_sg::track_default)),
	.track_s = const_cast<int32_t*>(std::data(luminance_limiter_sg::track_s)),
	.track_e = const_cast<int32_t*>(std::data(luminance_limiter_sg::track_e)),
	.func_proc = &luminance_limiter_sg::func_proc,
	.information = luminance_limiter_sg::information,
};

auto __stdcall GetFilterTable(void) {
	return &filter;
}

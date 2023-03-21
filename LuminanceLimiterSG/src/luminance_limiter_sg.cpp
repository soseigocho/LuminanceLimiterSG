#include "luminance_limiter_sg.h"

#include <array>
#include <limits>
#include <optional>


namespace luminance_limiter_sg {
	constexpr static inline auto name = "LuminanceLimiterSG";
	constexpr static inline auto track_n = 7u;
	constexpr static inline auto track_name =
		std::array<const char*, track_n>{ "è„å¿", "è„å¿Ëáíl", "â∫å¿", "â∫å¿Ëáíl", "ÉQÉCÉì", "ñæÇÈÇ≥", "à√Ç≥"};
	constexpr static inline auto track_default = std::array<int32_t, track_n>{ 4096, -256, 256, 0, 0, 0, 0 };
	constexpr static inline auto track_s = std::array<int32_t, track_n>{ 0, -4096, 0, 0, -4096, -4096, -4096 };
	constexpr static inline auto track_e = std::array<int32_t, track_n>{ 4096, 0, 4096, 4096, 4096, 4096, 4096 };
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

	constexpr static inline auto make_character(
		const NormalizedY top_limit, const NormalizedY top_threashold,
		const NormalizedY bottom_limit, const NormalizedY bottom_threashold,
		const NormalizedY top_peak, const NormalizedY bottom_peak)
	{
		auto top_coefficient = 1.0f;
		auto top_slice = 0.0f;
		if (top_peak >= top_limit)
		{
			top_coefficient =
				(top_limit - top_threashold) / (top_peak - top_threashold);
			top_slice = top_limit - top_peak * top_coefficient;
		}

		auto bottom_coefficient = 1.0f;
		auto bottom_slice = 0.0f;
		if (bottom_peak <= bottom_limit)
		{
			bottom_coefficient =
				(bottom_limit - bottom_threashold) / (bottom_peak - bottom_threashold);
			bottom_slice = bottom_limit - bottom_peak * bottom_coefficient;
		}

		return [=](const NormalizedY y) -> NormalizedY {
			if (y >= top_threashold)
			{
				return top_coefficient * y + top_slice;
			}
			else if (y < top_threashold && y > bottom_threashold)
			{
				return y;
			}
			else
			{
				return bottom_coefficient * y + bottom_slice;
			}
		};
	}

	template <typename F>
	constexpr static inline auto make_limit(
		const F& character,
		const NormalizedY top_limit,
		const NormalizedY bottom_limit)
	{
		return [=,&character](const NormalizedY y) -> NormalizedY {
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

	static inline BOOL func_proc(AviUtl::FilterPlugin* fp, AviUtl::FilterProcInfo* fpip)
	{
		auto normalized_y_buffer = NormalizedYBuffer(fpip);

		const auto orig_top = normalized_y_buffer.maximum();
		const auto orig_bottom = normalized_y_buffer.minimum();
		const auto top_diff = normalize_y(fp->track[5]);
		const auto bottom_diff = normalize_y(fp->track[6]);
		const auto scale = make_scale(orig_top, orig_bottom, top_diff, bottom_diff);

		const auto gain_val = normalize_y(fp->track[4]);
		const auto gain = make_gain(gain_val);

		const auto scale_and_gain = [&](NormalizedY y) {return gain(scale(y)); };

		const auto top_limit = normalize_y(fp->track[0]);
		const auto top_threashold = top_limit + normalize_y(fp->track[1]);
		const auto bottom_limit = normalize_y(fp->track[2]);
		const auto bottom_threashold = bottom_limit + normalize_y(fp->track[3]);
		const auto gained_top = scale_and_gain(orig_top);
		const auto gained_bottom = scale_and_gain(orig_bottom);

		const auto character =
			make_character(top_limit, top_threashold,
				bottom_limit, bottom_threashold,
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

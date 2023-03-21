#include "luminance_limiter_sg.h"

#include <array>


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
	NormalizedY normalize_y(T y) noexcept {
		return static_cast<float>(y) / y_max;
	}

	template <typename T>
	T denormalize_y(NormalizedY normalized_y) noexcept {
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
		return *std::max_element(this->buffer.begin(), this->buffer.end());
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
				(row+x)->y = denormalize_y<int16_t>(this->buffer[buf_idx]);
				buf_idx++;
			}
		}
		return true;
	}

	constexpr static inline auto stretch_scale(NormalizedY peak, NormalizedY diff, NormalizedY range) noexcept
	{
		const auto scaled = peak + diff;
		const auto scaled_range = scaled - peak;
		const auto scale = scaled_range / range;
		return scale;
	}

	constexpr static inline auto stretch_diff(NormalizedY threashold, NormalizedY scale, NormalizedY y) noexcept
	{
		const auto stretching_range = y - threashold;
		const auto stretched_range = stretching_range * scale;
		const auto stretched = stretched_range + threashold;
		const auto diff = stretched - y;
		return diff;
	}

	constexpr static inline auto make_scale(
		const NormalizedY orig_top, const NormalizedY orig_bottom,
		const NormalizedY top_diff, const NormalizedY bottom_diff)
	{
		const auto range = orig_top - orig_bottom;

		const auto top_scale = stretch_scale(orig_top, top_diff, range);
		const auto bottom_scale = stretch_scale(orig_bottom, bottom_diff, -range);

		return [=](NormalizedY y) -> NormalizedY {
			const auto upper_y_diff = stretch_diff(orig_bottom, top_scale, y);
			const auto lower_y_diff = stretch_diff(orig_top, bottom_scale, y);
			return y + upper_y_diff + lower_y_diff;
		};
	}

	constexpr static inline auto make_gain(NormalizedY gain)
	{
		return [=](NormalizedY y) {return y + gain; };
	}

	constexpr static inline auto make_character()
	{
		return 0;
	}

	constexpr static inline auto make_limit()
	{
		return 0;
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
		normalized_y_buffer.pixelwise_map(gain);

		const auto gained_top = normalized_y_buffer.maximum();
		const auto gained_bottom = normalized_y_buffer.minimum();

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

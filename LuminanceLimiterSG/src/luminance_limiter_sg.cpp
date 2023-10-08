/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#include "luminance_limiter_sg.h"
#include "limiter.h"
#include "peak_envelope_generator.h"

#include <array>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>


namespace luminance_limiter_sg {
	constexpr static inline auto name = "LuminanceLimiterSG";
	constexpr static inline auto track_n = 10u;
	constexpr static inline auto track_name =
		std::array<const char*, track_n>{ "è„å¿", "è„å¿Ëáíl", "â∫å¿", "â∫å¿Ëáíl", "ï‚ä‘”∞ƒﬁ", "éùë±", "ó]âC", "µÃæØƒ", "ñæÇÈÇ≥", "à√Ç≥"};
	constexpr static inline auto track_default = std::array<int32_t, track_n>{ 4096, -1024, 256, 512, 0, 4, 8, -128, 512, -64 };
	constexpr static inline auto track_s = std::array<int32_t, track_n>{ 0, -4096, 0, 0, 0, 0, 0, -4096, -4096, -4096 };
	constexpr static inline auto track_e = std::array<int32_t, track_n>{ 4096, 0, 4096, 4096, 2, 256, 256, 4096, 4096, 4096 };
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

	static auto peak_envelope_generator = PeakEnvelopeGenerator();
	static auto limiter = Limiter();

	static inline BOOL func_proc(AviUtl::FilterPlugin* fp, AviUtl::FilterProcInfo* fpip)
	{
		auto normalized_y_buffer = NormalizedYBuffer(fpip);

		const auto orig_top = normalized_y_buffer.maximum();
		const auto orig_bottom = normalized_y_buffer.minimum();
		const auto top_diff = normalize_y(fp->track[8]);
		const auto bottom_diff = normalize_y(fp->track[9]);
		limiter.update_scale(orig_top, orig_bottom, top_diff, bottom_diff);

		const auto gain_val = normalize_y(fp->track[7]);
		limiter.update_gain(gain_val);

		const auto top_limit = normalize_y(fp->track[0]);
		const auto top_threshold_diff = normalize_y(fp->track[1]);
		const auto bottom_limit = normalize_y(fp->track[2]);
		const auto bottom_threshold_diff = normalize_y(fp->track[3]);
		peak_envelope_generator.set_limit(top_limit, bottom_limit);

		const auto sustain = static_cast<uint32_t>(fp->track[5]);
		peak_envelope_generator.set_sustain(sustain);
		const auto release = static_cast<uint32_t>(fp->track[6]);
		peak_envelope_generator.set_release(release);

		const auto gained_top = limiter.scale_and_gain(orig_top);
		const auto gained_bottom = limiter.scale_and_gain(orig_bottom);
		const auto [enveloped_top, enveloped_bottom] =
			peak_envelope_generator.update_and_get_envelope_peaks(gained_top, gained_bottom);

		const auto limit_character_interpolation_mode = static_cast<InterpolationMode>(fp->track[4]);
		limiter.update_limiter(
			top_limit, top_threshold_diff,
			bottom_limit, bottom_threshold_diff,
			enveloped_top, enveloped_bottom,
			limit_character_interpolation_mode);

		normalized_y_buffer.pixelwise_map([](const NormalizedY y) -> NormalizedY { return limiter.limit(limiter.scale_and_gain(y)); });

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

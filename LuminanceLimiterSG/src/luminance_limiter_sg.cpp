/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#include "luminance_limiter_sg.h"
#include "ruck_unit.h"

#include <array>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>


namespace luminance_limiter_sg {
	constexpr static inline auto name = "LuminanceLimiterSG";
	
	constexpr static inline auto track_n = 11u;
	constexpr static inline auto track_name =
		std::array<const char*, track_n>{ "ID", "è„å¿", "è„å¿Ëáíl", "â∫å¿", "â∫å¿Ëáíl", "ï‚ä‘”∞ƒﬁ", "éùë±(ms)", "ó]âC(ms)", "µÃæØƒ", "ñæÇÈÇ≥", "à√Ç≥"};
	constexpr static inline auto track_default = std::array<int32_t, track_n>{ 0, 4096, -1024, 256, 512, 0, 100, 400, -128, 512, -64 };
	constexpr static inline auto track_s = std::array<int32_t, track_n>{ 0, 0, -4096, 0, 0, 0, 0, 0, -4096, -4096, -4096 };
	constexpr static inline auto num_or_rucks = 16UL;
	constexpr static inline auto track_e = std::array<int32_t, track_n>{ num_or_rucks, 4096, 0, 4096, 4096, 2, 256, 256, 4096, 4096, 4096 };

	constexpr static inline auto on = 1UL;
	constexpr static inline auto off = 0UL;
	constexpr static inline auto check_n = 1u;
	constexpr static inline auto check_name = std::array<const char*, check_n>{"Limiter Mode"};
	constexpr static inline auto check_default = std::array<int32_t, check_n>{off};

	constexpr static inline auto information = "LuminanceLimiterSG v0.2.0 by ëeêªåﬁí∑";

	constexpr static inline auto y_max = 4096.0F;
	constexpr static inline auto y_min = 0.0F;

	static std::optional<float> fps = std::nullopt;

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

	static std::unique_ptr<std::array<std::unique_ptr<RuckUnit>, num_or_rucks>> effects_ruck;
	static auto current_frame = 0UL;

	const static inline bool check_and_fill_effects_ruck(const int32_t effects_id)
	{
		if (effects_ruck->size() < effects_id + 1)
		{
			throw std::out_of_range("the effects_ruck has smaller elements than the input.");
		}

		if (!(*effects_ruck)[effects_id])
		{
			(*effects_ruck)[effects_id].reset(new RuckUnit());
			return false;
		}

		if (!static_cast<bool>((*effects_ruck)[effects_id]))
		{
			throw std::domain_error("the effects_ruck member has not been initialized.");
		}

		return true;
	}

	static inline BOOL func_proc(AviUtl::FilterPlugin* fp, AviUtl::FilterProcInfo* fpip)
	{
		auto normalized_y_buffer = NormalizedYBuffer(fpip);

		const auto orig_top = normalized_y_buffer.maximum();
		const auto orig_bottom = normalized_y_buffer.minimum();
		const auto top_diff = normalize_y(fp->track[9]);
		const auto bottom_diff = normalize_y(fp->track[10]);
		if (!effects_ruck)
		{
			throw std::domain_error("effects_ruck has not been initialized.");
		}

		if (current_frame < fpip->frame)
		{
			for (auto&& elem : (*effects_ruck))
			{
				if (elem)
				{
					if (elem->used)
					{
						elem.reset(nullptr);
					}
					else
					{
						elem->used = false;
					}
				}
			}
			current_frame = fpip->frame;
		}

		const auto effects_id = static_cast<uint32_t>(fp->track[0]);
		AviUtl::FileInfo fi;
		fp->exfunc->get_file_info(fpip->editp, &fi);
		fps = static_cast<float>(fi.video_rate);
		if (!check_and_fill_effects_ruck(effects_id))
		{
			const auto top_limit = normalize_y(fp->track[1]);
			const auto bottom_limit = normalize_y(fp->track[3]);
			((*effects_ruck)[effects_id])->peak_envelope_generator.set_limit(top_limit, bottom_limit);

			if (!fps.has_value())
			{
				throw std::runtime_error("Fpms has not initialized.");
			}
			const auto sustain = static_cast<uint32_t>(std::floor(static_cast<float>(fp->track[6]) * fps.value() / 1000.0f ));
			((*effects_ruck)[effects_id])->peak_envelope_generator.set_sustain(sustain);

			const auto release = std::ceil(static_cast<float>(fp->track[7]) * fps.value() / 1000.0f );
			((*effects_ruck)[effects_id])->peak_envelope_generator.set_release(release);

			const auto gain_val = normalize_y(fp->track[8]);
			((*effects_ruck)[effects_id])->limiter.update_gain(gain_val);
		}

		(*effects_ruck)[effects_id]->used = true;

		((*effects_ruck)[effects_id])->limiter.update_scale(orig_top, orig_bottom, top_diff, bottom_diff);

		const auto top_limit = normalize_y(fp->track[1]);
		const auto top_threshold_diff = normalize_y(fp->track[2]);
		const auto bottom_limit = normalize_y(fp->track[3]);
		const auto bottom_threshold_diff = normalize_y(fp->track[4]);

		const auto gained_top = ((*effects_ruck)[effects_id])->limiter.scale_and_gain(orig_top);
		const auto gained_bottom = ((*effects_ruck)[effects_id])->limiter.scale_and_gain(orig_bottom);
		const auto [enveloped_top, enveloped_bottom] =
			((*effects_ruck)[effects_id])->peak_envelope_generator.update_and_get_envelope_peaks(gained_top, gained_bottom);

		const auto limit_character_interpolation_mode = static_cast<InterpolationMode>(fp->track[5]);
		((*effects_ruck)[effects_id])->limiter.update_limiter(
			top_limit, top_threshold_diff,
			bottom_limit, bottom_threshold_diff,
			enveloped_top, enveloped_bottom,
			limit_character_interpolation_mode);

		normalized_y_buffer.pixelwise_map(
			[=](const NormalizedY y) -> NormalizedY
			{ return ((*effects_ruck)[effects_id])->limiter.limit(((*effects_ruck)[effects_id])->limiter.scale_and_gain(y)); });

		normalized_y_buffer.render(fpip);

		return true;
	}

	static inline BOOL func_init(AviUtl::FilterPlugin* fp)
	{
		effects_ruck = std::make_unique<std::array<std::unique_ptr<RuckUnit>, num_or_rucks>>();
		return static_cast<bool>(effects_ruck);
	}

	static inline BOOL func_update(AviUtl::FilterPlugin* fp, AviUtl::FilterPluginDLL::UpdateStatus status)
	{
		if (AviUtl::detail::FilterPluginUpdateStatus::Track <= status &&
			status < AviUtl::detail::FilterPluginUpdateStatus::Check)
		{
			const auto track =
				static_cast<std::underlying_type<AviUtl::FilterPluginDLL::UpdateStatus>::type>(status)
				- static_cast<std::underlying_type<AviUtl::detail::FilterPluginUpdateStatus>::type>(AviUtl::detail::FilterPluginUpdateStatus::Track);

			const auto effects_id = static_cast<uint32_t>(fp->track[0]);
			check_and_fill_effects_ruck(effects_id);

			switch (track)
			{
			case 1U:
			{
				break;
			}
			case 2U:
			{
				const auto top_limit = normalize_y(fp->track[1]);
				const auto bottom_limit = normalize_y(fp->track[3]);
				((*effects_ruck)[effects_id])->peak_envelope_generator.set_limit(top_limit, bottom_limit);
				break;
			}
			case 4U:
			{
				const auto top_limit = normalize_y(fp->track[1]);
				const auto bottom_limit = normalize_y(fp->track[3]);
				((*effects_ruck)[effects_id])->peak_envelope_generator.set_limit(top_limit, bottom_limit);
				break;
			}
			case 7U:
			{
				const auto sustain = static_cast<uint32_t>(fp->track[6]);
				((*effects_ruck)[effects_id])->peak_envelope_generator.set_sustain(sustain);
				break;
			}
			case 8U:
			{
				const auto release = static_cast<uint32_t>(fp->track[7]);
				((*effects_ruck)[effects_id])->peak_envelope_generator.set_release(release);
				break;
			}
			case 9U:
			{
				const auto gain_val = normalize_y(fp->track[8]);
				((*effects_ruck)[effects_id])->limiter.update_gain(gain_val);
				break;
			}
			default:
				break;
			}
		}
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
	.check_n = luminance_limiter_sg::check_n,
	.check_name = const_cast<const char**>(std::data(luminance_limiter_sg::check_name)),
	.check_default = const_cast<int32_t*>(std::data(luminance_limiter_sg::check_default)),
	.func_proc = &luminance_limiter_sg::func_proc,
	.func_init = &luminance_limiter_sg::func_init,
	.func_update = &luminance_limiter_sg::func_update,
	.information = luminance_limiter_sg::information,
};

auto __stdcall GetFilterTable(void) {
	return &filter;
}

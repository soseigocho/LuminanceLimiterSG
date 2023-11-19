/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


//#include "luminance_limiter_sg.h"

#include <array>
#include <optional>

#include "dithering_filter.h"
#include "luminance.h"
#include "project_parameter.h"
#include "rack.h"


namespace luminance_limiter_sg {
	constexpr static inline auto name = "LuminanceLimiterSG";

	constexpr static inline auto track_n = 8u;
	constexpr static inline auto track_name = std::array<const char*, track_n>
	{
		"ID",
			"è„å¿(L)", "Ëáíl1", "Ëáíl2", "â∫å¿(L)",
			"S[ms]", "R[ms]",
			"ï‚ä‘”∞ƒﬁ"
	};
	constexpr static inline auto track_default = std::array<int32_t, track_n>
	{
		0,
			4096, 4095, 1, 0,
			1, 0,
			0
	};
	constexpr static inline auto track_s = std::array<int32_t, track_n>
	{
		0,
			3, 2, 1, 0,
			1, 0,
			0
	};
	constexpr static inline auto track_e = std::array<int32_t, track_n>
	{
		num_or_racks,
			4096, 4095, 4094, 4093,
			4096, 4096,
			2
	};

	constexpr static inline auto information = "LuminanceLimiterSG v0.2.0 by ëeêªåﬁí∑";

	static auto rack = Rack();
	static std::optional<Buffer> processing_buffer = std::nullopt;
	static auto dithering_filter = DitheringFilter();

	static inline BOOL func_proc(AviUtl::FilterPlugin* fp, AviUtl::FilterProcInfo* fpip)
	{
		ProjectParameter::init(static_cast<double>(get_fps(fp, fpip)));

		rack.gc(static_cast<uint32_t>(fpip->frame));

		auto effector_id = static_cast<uint32_t>(fp->track[0]);
		
		rack.check_and_set_effector(effector_id, fp);
		rack[effector_id]->used();

		processing_buffer = processing_buffer ? processing_buffer : Buffer(fpip->max_w, fpip->max_h);
		processing_buffer->fetch_image(fpip->w, fpip->h, static_cast<AviUtl::PixelYC*>(fpip->ycp_edit));

		rack[effector_id]->fetch_trackbar_and_buffer(fp, *processing_buffer);

		processing_buffer->pixelwise_map(rack[effector_id]->effect());
		processing_buffer->render(fpip->w, fpip->h, static_cast<AviUtl::PixelYC*>(fpip->ycp_edit), dithering_filter.dither);

		return true;
	}

	static inline BOOL func_update(AviUtl::FilterPlugin* fp, AviUtl::FilterPluginDLL::UpdateStatus status)
	{
		if (AviUtl::detail::FilterPluginUpdateStatus::Track <= status &&
			status < AviUtl::detail::FilterPluginUpdateStatus::Check)
		{
			const auto track =
				static_cast<std::underlying_type<AviUtl::FilterPluginDLL::UpdateStatus>::type>(status)
				- static_cast<std::underlying_type<AviUtl::detail::FilterPluginUpdateStatus>::type>(AviUtl::detail::FilterPluginUpdateStatus::Track);

			const auto effector_id = static_cast<uint32_t>(fp->track[0]);
			rack.check_and_set_effector(effector_id, fp);
			rack[effector_id]->update_from_trackbar(fp, track);
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
	.func_proc = &luminance_limiter_sg::func_proc,
	.func_update = &luminance_limiter_sg::func_update,
	.information = luminance_limiter_sg::information,
};

auto __stdcall GetFilterTable(void) {
	return &filter;
}

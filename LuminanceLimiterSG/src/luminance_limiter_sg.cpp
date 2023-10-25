/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


//#include "luminance_limiter_sg.h"

#include <array>
#include <optional>

#include "luminance.h"
#include "project_parameter.h"
#include "rack.h"


namespace luminance_limiter_sg {
	constexpr static inline auto name = "LuminanceLimiterSG";
	
	constexpr static inline auto track_n = 15u;
	constexpr static inline auto track_name = std::array<const char*, track_n>{
		"ID",
		"ñæ(in)", "à√(in)", "µÃæØƒ(in)",
		"Ëáíl1", "Ëáíl2",
		"A(C)[ms]","S[ms]", "R[ms]",
		"à≥èkó¶(C)",
		"è„å¿(L)", "â∫å¿(L)",
		"ñæ(out)", "à√(out)", "µÃæØƒ(out)" };
	constexpr static inline auto track_default = std::array<int32_t, track_n>{
		0,
		0, 0, 0,
		4096, 0,
		0, 0, 0,
		1,
		4096, 0,
		0, 0, 0 };
	constexpr static inline auto track_s = std::array<int32_t, track_n>{
		0,
		-4096, -4096, -4096,
		0, 0,
		0, 0, 0,
		1,
		0, -4096,
		-4096, -4096, -4096 };
	constexpr static inline auto track_e = std::array<int32_t, track_n>{
		num_or_racks,
		4096, 4096, 4096,
		4096, 4096,
		4096, 4096, 4096,
		8,
		4096, 4096,
		4096, 4096, 4096};

	constexpr static inline auto on = 1UL;
	constexpr static inline auto off = 0UL;
	constexpr static inline auto check_n = 1u;
	constexpr static inline auto check_name = std::array<const char*, check_n>{"Limiter Mode"};
	constexpr static inline auto check_default = std::array<int32_t, check_n>{on};

	constexpr static inline auto information = "LuminanceLimiterSG v0.2.0 by ëeêªåﬁí∑";

	static Rack rack = Rack();
	static std::optional<Buffer> processing_buffer = std::nullopt;

	static inline BOOL func_proc(AviUtl::FilterPlugin* fp, AviUtl::FilterProcInfo* fpip)
	{
		if (!ProjectParameter::fps())
		{
			AviUtl::FileInfo fi;
			fp->exfunc->get_file_info(fpip->editp, &fi);
			ProjectParameter::fps() = static_cast<float>(fi.video_rate);
		}

		if (!processing_buffer)
		{
			processing_buffer = Buffer(fpip->max_w, fpip->max_h);
		}

		if (rack.is_first_time(static_cast<uint32_t>(fpip->frame)))
		{
			rack.gc();
		}

		auto effector_id = static_cast<unsigned int>(fp->track[0]);
		auto processing_mode = static_cast<ProcessingMode>(fp->check[0]);

		if (!rack[effector_id])
		{
			rack.set_effector(effector_id, processing_mode, fp);
		}

		std::visit([](auto& x) { x.used(); }, *rack[effector_id].value());

		processing_buffer.value().fetch_image(fpip->w, fpip->h, static_cast<AviUtl::PixelYC*>(fpip->ycp_edit));

		std::visit([&](auto& x) { x.fetch_trackbar_and_buffer(fp, processing_buffer.value()); }, *rack[effector_id].value());

		processing_buffer.value().pixelwise_map(std::visit([](const auto& x) { return x.effect(); }, *rack[effector_id].value()));
		processing_buffer.value().render(fpip->w, fpip->h, static_cast<AviUtl::PixelYC*>(fpip->ycp_edit));

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
			std::visit([&](auto& x) { x.update_from_trackbar(fp, track); }, *rack[effector_id].value());
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
	.func_update = &luminance_limiter_sg::func_update,
	.information = luminance_limiter_sg::information,
};

auto __stdcall GetFilterTable(void) {
	return &filter;
}

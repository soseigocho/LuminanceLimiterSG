#pragma once

#include "aviutl/filter.hpp"

namespace luminance_limiter_sg
{
	constexpr inline auto id = [](auto x) -> auto { return x; };

	const static inline auto get_fps(const AviUtl::FilterPlugin* const fp, const AviUtl::FilterProcInfo* const fpip) noexcept
	{
		AviUtl::FileInfo fi;
		fp->exfunc->get_file_info(fpip->editp, &fi);
		return fi.video_rate;
	}
}
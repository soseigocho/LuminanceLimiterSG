#include "luminance_limiter_sg.h"

#include <aviutl/filter.hpp>

#include <algorithm>
#include <array>


namespace luminance_limiter_sg {

	constexpr static inline auto track_n = 5u;
	constexpr static inline auto track_name = std::array<const char*, track_n>{ "ãŒÀ", "ãŒÀè‡’l", "‰ºŒÀ", "‰ºŒÀè‡’l", "ƒQƒCƒ“"};
	constexpr static inline auto track_default = std::array<int32_t, track_n>{ 4096, -2048, 1024, 0, 0 };
	constexpr static inline auto track_s = std::array<int32_t, track_n>{ 0, -4096, 0, 0, -4096 };
	constexpr static inline auto track_e = std::array<int32_t, track_n>{ 4096, 0, 4096, 4096, 4096 };
}


constexpr AviUtl::FilterPluginDLL filter{
	.flag = AviUtl::FilterPlugin::Flag::ExInformation,
	.name = "LuminanceLimiterSG",
	.track_n = luminance_limiter_sg::track_n,
	.track_name = const_cast<const char**>(std::data(luminance_limiter_sg::track_name)),
	.track_default = const_cast<int32_t*>(std::data(luminance_limiter_sg::track_default)),
	.track_s = const_cast<int32_t*>(std::data(luminance_limiter_sg::track_s)),
	.track_e = const_cast<int32_t*>(std::data(luminance_limiter_sg::track_e)),
	.information = "LuminanceLimiterSG v0.2.0 by ‘e»ŒŞ’·",
};

auto __stdcall GetFilterTable(void) {
	return &filter;
}

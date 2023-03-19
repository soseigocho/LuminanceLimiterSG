#include "luminance_limiter_sg.h"

#include <aviutl/filter.hpp>

AviUtl::FilterPluginDLL filter{
	.flag = AviUtl::FilterPlugin::Flag::AlwaysActive,
	.name = "LuminanceLimiterSG",
};

auto __stdcall GetFilterTable(void) {
	return &filter;
}

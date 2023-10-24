/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once

#include <concepts>

#include "aviutl/FilterPlugin.hpp"

namespace luminance_limiter_sg
{
	template<typename T>
	concept Effector = requires (T a, AviUtl::FilterPlugin * fp, const Buffer & buffer)
	{
		{ new T(fp) };
		{ a.effect() } noexcept -> std::invocable;
		{ a.fetch_trackbar_and_buffer(fp, buffer) };
		{ a.used() } noexcept;
		{ a.reset() } noexcept;
		{ a.is_using() } noexcept;
	};
}
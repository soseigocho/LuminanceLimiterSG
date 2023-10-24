/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once

#include <variant>

#include "aviutl/FilterPlugin.hpp"
#include "buffer.h"
#include "effector.h"


namespace luminance_limiter_sg
{
	class IRackUnit
	{
	public:
		virtual const void fetch_trackbar_and_buffer(const AviUtl::FilterPlugin* const fp, const Buffer& buffer) = 0;
		virtual const void update_from_trackbar(const AviUtl::FilterPlugin* const fp, const uint32_t track) = 0;

		virtual const void used() noexcept = 0;
		virtual const void reset() noexcept = 0;

		virtual const bool is_using() const noexcept = 0;
	};

	template<typename T>
	concept CRackUnit = requires
	{
		Effector<T>;
		std::derived_from<T, IRackUnit>;
	};

	template<typename ...Args>
	using RackUnit = std::variant<Args...>;
}
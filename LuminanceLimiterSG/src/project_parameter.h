/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once

#include <optional>

#include "aviutl/filter.hpp"


namespace luminance_limiter_sg
{
	class ProjectParameter
	{
	public:
		constexpr static inline void init(auto project_fps) noexcept
		{
			if (fps)
			{
				fps = project_fps;
			}
		}

		constexpr static inline const auto& get_fps() noexcept
		{
			return fps;
		}
	private:
		static inline std::optional<double> fps = std::nullopt;
	};
}
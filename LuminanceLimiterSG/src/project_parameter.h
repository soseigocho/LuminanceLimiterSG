/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once

#include <optional>


namespace luminance_limiter_sg
{
	struct ProjectParameter
	{
		static std::optional<float>& fps() noexcept
		{
			static std::optional<float> fps_data;
			return fps_data;
		}
	};
}
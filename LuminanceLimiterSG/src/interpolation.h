/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once

#include <functional>
#include <optional>
#include <vector>

namespace luminance_limiter_sg
{
	enum class InterpolationMode : int32_t
	{
		Linear,
		Lagrange,
		Spline
	};

	constexpr std::optional<int> inner_binary_search(const std::vector<float>& xs, const float x, const int max_idx, const int min_idx);
	constexpr auto binary_search(const std::vector<float>& xs, const float x);

	const std::function<float(float)> linear_interp(const std::vector<float>&& xs, const std::vector<float>&& ys);

	const std::function<float(float)> lagrange_interp(const std::vector<float>&& xs, const std::vector<float>&& ys);

	constexpr auto tdma(const std::vector<float>& a, const std::vector<float>& b, const std::vector<float>& c, const std::vector<float>& d);
	const std::function<float(float)> spline_interp(const std::vector<float>&& xs, const std::vector<float>&& ys);

}

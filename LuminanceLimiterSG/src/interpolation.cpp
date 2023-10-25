/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#include "interpolation.h"

#include <stdexcept>


namespace luminance_limiter_sg
{
	constexpr std::optional<int> inner_binary_search(const std::vector<float>& xs, const float x, const int max_idx, const int min_idx)
	{
		if (max_idx < min_idx)
		{
			return std::nullopt;
		}

		auto mid_idx = min_idx + (max_idx - min_idx) / 2;
		if (xs[mid_idx] > x)
		{
			return inner_binary_search(xs, x, min_idx, mid_idx - 1);
		}
		else if (xs[mid_idx] < x)
		{
			return inner_binary_search(xs, x, mid_idx + 1, max_idx);
		}
		else
		{
			return mid_idx;
		}
	}

	constexpr auto binary_search(const std::vector<float>& xs, const float x)
	{
		return inner_binary_search(xs, x, 0, xs.size() - 1);
	}

	const std::function<float(float)> linear_interp(const std::vector<float>&& xs, const std::vector<float>&& ys)
	{
		if (xs.size() != ys.size())
		{
			throw std::runtime_error("Error: xs and ys must have the same size.");
		}

		if (xs.size() < 2)
		{
			throw std::runtime_error("Error: xs and ys must have at least 2 elements.");
		}

		auto n = xs.size();
		std::vector<float> slopes(n - 1);
		for (auto i = 0; i < n - 1; ++i)
		{
			slopes[i] = (ys[i + 1] - ys[i]) / (xs[i + 1] - xs[i]);
		}

		return [=](float x) -> float {
			const auto idx = binary_search(xs, x).value_or((x > xs[0]) ? 0 : (xs.size() - 1));
			return ys[idx] + slopes[idx] * (x - xs[idx]);
			};
	}

	const std::function<float(float)> lagrange_interp(const std::vector<float>&& xs, const std::vector<float>&& ys)
	{
		if (xs.size() != ys.size())
		{
			throw std::runtime_error("Error: xs and ys must have the same size.");
		}

		if (xs.size() < 2)
		{
			throw std::runtime_error("Error: xs and ys must have at least 2 elements.");
		}

		return [=](float x) {
			auto sum = 0.0f;
			for (auto i = 0; i < xs.size(); ++i)
			{
				auto prod = 1.0f;
				for (auto j = 0; j < xs.size(); ++j)
				{
					if (i != j)
					{
						prod *= (x - xs[j]) / (xs[i] - xs[j]);
					}
				}
				sum += ys[i] * prod;
			}
			return sum;
			};
	}

	constexpr auto tdma(const std::vector<float>& a, const std::vector<float>& b, const std::vector<float>& c, const std::vector<float>& d)
	{
		auto n = a.size() - 1;
		auto p = std::vector<float>(a.size());
		auto q = std::vector<float>(a.size());

		p[0] = -b[0] / a[0];
		q[0] = d[0] / a[0];

		for (auto i = 1; i < a.size(); i++)
		{
			p[i] = -b[i] / (a[i] + c[i] * p[i - 1]);
			q[i] = (d[i] - c[i] * q[i - 1]) / (a[i] + c[i] * p[i - 1]);
		}

		auto x = std::vector<float>(a.size());
		x[n] = q[n];
		for (auto i = n - 1; i > -1; i--)
		{
			x[i] = p[i] * x[i + 1] + q[i];
		}

		return x;
	}

	const std::function<float(float)> spline_interp(const std::vector<float>&& xs, const std::vector<float>&& ys)
	{
		if (xs.size() != ys.size())
		{
			throw std::runtime_error("Error: xs and ys must have the same size.");
		}

		if (xs.size() < 2)
		{
			throw std::runtime_error("Error: xs and ys must have at least 2 elements.");
		}

		const auto n = xs.size() - 1;

		auto as = std::vector<float>(xs.size());
		for (auto i = 0; i < xs.size(); i++)
		{
			as[i] = ys[i];
		}

		auto hs = std::vector<float>(n);
		for (auto i = 0; i < n; i++)
		{
			hs[i] = xs[i + 1] - xs[i];
		}

		auto aas = std::vector<float>(xs.size());
		for (auto i = 0; i < xs.size(); i++)
		{
			if (i == 0)
			{
				aas[i] = 1;
			}
			else if (i == n)
			{
				aas[i] = 0;
			}
			else
			{
				aas[i] = 2 * (hs[i - 1] + hs[i]);
			}
		}

		auto abs = std::vector<float>(xs.size());
		for (auto i = 0; i < xs.size(); i++)
		{
			if (i == 0)
			{
				abs[i] = 0;
			}
			else if (i == n)
			{
				abs[i] = 1;
			}
			else
			{
				abs[i] = hs[i];
			}
		}

		auto acs = std::vector<float>(xs.size());
		for (auto i = 0; i < xs.size(); i++)
		{
			if (i == 0)
			{
				acs[i] = 0;
			}
			else
			{
				acs[i] = hs[i - 1];
			}
		}

		auto prod_b = std::vector<float>(xs.size());
		for (auto i = 0; i < xs.size(); i++)
		{
			if (i == 0 || i == n)
			{
				prod_b[i] = 0;
			}
			else
			{
				prod_b[i] = 3 * ((as[i + 1] - as[i]) / hs[i] - (as[i] - as[i - 1]) / hs[i - 1]);
			}
		}

		auto cs = tdma(aas, abs, acs, prod_b);

		auto bs = std::vector<float>(xs.size());
		auto ds = std::vector<float>(xs.size());
		for (auto i = 0; i <= n; i++)
		{
			if (i == n)
			{
				bs[i] = 0.0;
				ds[i] = 0.0;
			}
			else
			{
				bs[i] = (as[i + 1] - as[i]) / hs[i] - hs[i] * (cs[i + 1] + 2 * cs[i]) / 3;
				ds[i] = (cs[i + 1] - cs[i]) / (3.0 * hs[i]);
			}
		}

		return [=](float x) {
			auto opt_i = binary_search(xs, x);

			auto i = opt_i.value_or((x > xs[0]) ? 0 : (xs.size() - 1));

			auto dt = x - xs[i];
			return as[i] + (bs[i] + (cs[i] + ds[i] * dt) * dt) * dt;
			};
	}
}
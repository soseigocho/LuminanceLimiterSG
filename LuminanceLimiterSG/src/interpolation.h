/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once

#include <concepts>
#include <optional>
#include <stdexcept>
#include <vector>

namespace luminance_limiter_sg
{
	enum class InterpolationMode : int32_t
	{
		Linear,
		Lagrange,
		Spline
	};

	constexpr static inline auto binary_search(const std::vector<double>& xs, const double x) noexcept
	{
		auto left = -1;
		auto right = xs.size() - 1;

		while (right - left > 1)
		{
			const auto mid = left + (right - left) / 2;
			if (xs[mid] > x)
			{
				right = mid;
			}
			else
			{
				left = mid;
			}
		}

		return right;
	}

	const static inline std::regular_invocable<double> auto linear_interp(const std::vector<double>&& xs, const std::vector<double>&& ys)
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
		std::vector<double> slopes(n - 1);
		for (auto i = 0; i < n - 1; ++i)
		{
			slopes[i] = (ys[i + 1] - ys[i]) / (xs[i + 1] - xs[i]);
		}

		return [=](double x) -> double {
			auto idx = binary_search(xs, x);
			idx = idx >= xs.size() - 1 ? xs.size() - 2 : idx;
			return ys[idx] + slopes[idx] * (x - xs[idx]);
			};
	};

	const static inline std::regular_invocable<double> auto lagrange_interp(const std::vector<double>&& xs, const std::vector<double>&& ys)
	{
		if (xs.size() != ys.size())
		{
			throw std::runtime_error("Error: xs and ys must have the same size.");
		}

		if (xs.size() < 2)
		{
			throw std::runtime_error("Error: xs and ys must have at least 2 elements.");
		}

		return [=](double x) {
			auto sum = 0.0;
			for (auto i = 0; i < xs.size(); ++i)
			{
				auto prod = 1.0;
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

	constexpr static inline auto tdma(const std::vector<double>& a, const std::vector<double>& b, const std::vector<double>& c, const std::vector<double>& d)
	{
		auto n = a.size() - 1;
		auto p = std::vector<double>(a.size());
		auto q = std::vector<double>(a.size());

		p[0] = -b[0] / a[0];
		q[0] = d[0] / a[0];

		for (auto i = 1; i < a.size(); i++)
		{
			p[i] = -b[i] / (a[i] + c[i] * p[i - 1]);
			q[i] = (d[i] - c[i] * q[i - 1]) / (a[i] + c[i] * p[i - 1]);
		}

		auto x = std::vector<double>(a.size());
		x[n] = q[n];
		for (auto i = n - 1; i > -1; i--)
		{
			x[i] = p[i] * x[i + 1] + q[i];
		}

		return x;
	}

	const static inline std::regular_invocable<double> auto spline_interp(const std::vector<double>&& xs, const std::vector<double>&& ys)
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

		auto as = std::vector<double>(xs.size());
		for (auto i = 0; i < xs.size(); i++)
		{
			as[i] = ys[i];
		}

		auto hs = std::vector<double>(n);
		for (auto i = 0; i < n; i++)
		{
			hs[i] = xs[i + 1] - xs[i];
		}

		auto aas = std::vector<double>(xs.size());
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

		auto abs = std::vector<double>(xs.size());
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

		auto acs = std::vector<double>(xs.size());
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

		auto prod_b = std::vector<double>(xs.size());
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

		auto bs = std::vector<double>(xs.size());
		auto ds = std::vector<double>(xs.size());
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

		return [=](double x) {
			auto idx = binary_search(xs, x);
			idx = idx >= xs.size() ? xs.size() - 1 : idx;
			auto dt = x - xs[idx];
			return as[idx] + (bs[idx] + (cs[idx] + ds[idx] * dt) * dt) * dt;
			};
	}

}

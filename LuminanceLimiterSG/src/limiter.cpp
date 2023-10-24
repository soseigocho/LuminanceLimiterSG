/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/

#include "limiter.h"

#include "luminance_limiter_sg.h"
#include "luminance.h"
#include "project_parameter.h"

namespace luminance_limiter_sg {
	constexpr static inline auto stretch_scale(const NormalizedY peak, const NormalizedY threashold, const NormalizedY diff) noexcept
	{
		const auto range = peak - threashold;
		const auto scaled_range = peak + diff - threashold;
		const auto scale = scaled_range / range;
		return scale;
	}

	constexpr static inline auto stretch_diff(const NormalizedY threashold, const NormalizedY scale, const NormalizedY y) noexcept
	{
		const auto stretching_range = y - threashold;
		const auto stretched_range = stretching_range * scale;
		const auto stretched = stretched_range + threashold;
		const auto diff = stretched - y;
		return diff;
	}

	const inline std::function<NormalizedY(NormalizedY)> make_scale(
		const NormalizedY orig_top, const NormalizedY orig_bottom,
		const NormalizedY top_diff, const NormalizedY bottom_diff) noexcept
	{
		const auto top_scale = stretch_scale(orig_top, orig_bottom, top_diff);
		const auto bottom_scale = stretch_scale(orig_bottom, orig_top, bottom_diff);

		return [=](NormalizedY y) -> NormalizedY {
			const auto upper_y_diff = stretch_diff(orig_bottom, top_scale, y);
			const auto lower_y_diff = stretch_diff(orig_top, bottom_scale, y);
			return y + upper_y_diff + lower_y_diff;
			};
	}

	const inline std::function<NormalizedY(NormalizedY)> make_gain(const NormalizedY gain) noexcept
	{
		return [=](NormalizedY y) {return y + gain; };
	}

	const inline std::function<float(float)> linear_interp(const std::vector<float>&& xs, const std::vector<float>&& ys)
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
			if (x < xs[0] || x > xs[n - 1])
			{
				throw std::range_error("Error: x is out of range.");
			}

			auto left = 0;
			auto right = n - 1;
			while (left + 1 < right)
			{
				const auto mid = (left + right) / 2;
				if (xs[mid] <= x)
				{
					left = mid;
				}
				else
				{
					right = mid;
				}
			}

			const auto idx = left;
			return ys[idx] + slopes[idx] * (x - xs[idx]);
			};
	}

	const inline std::function<float(float)> lagrange_interp(const std::vector<float>&& xs, const std::vector<float>&& ys)
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

	constexpr static inline auto tdma(const std::vector<float>& a, const std::vector<float>& b, const std::vector<float>& c, const std::vector<float>& d)
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

	constexpr static inline std::optional<int> inner_binary_search(const std::vector<float>& xs, const float x, const int max_idx, const int min_idx)
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

	constexpr static inline auto binary_search(const std::vector<float>& xs, const float x)
	{
		return inner_binary_search(xs, x, 0, xs.size() - 1);
	}

	const inline std::function<float(float)> spline_interp(const std::vector<float>&& xs, const std::vector<float>&& ys)
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

	const inline std::function<
		std::function<float(float)>(
			NormalizedY, NormalizedY,
			NormalizedY, NormalizedY,
			NormalizedY, NormalizedY)> select_character(InterpolationMode mode)
	{
		switch (mode)
		{
		case InterpolationMode::Linear:
			return make_linear_character;
		case InterpolationMode::Lagrange:
			return make_lagrange_character;
		case InterpolationMode::Spline:
			return make_spline_character;
		default:
			throw std::runtime_error("Error: Illegal interpolation mode.");
		}
	}

	Limiter::Limiter(const AviUtl::FilterPlugin* const fp)
	{
		const auto top_limit = normalize_y(fp->track[1]);
		const auto bottom_limit = normalize_y(fp->track[3]);
		peak_envelope_generator.set_limit(top_limit, bottom_limit);

		if (!ProjectParameter::fps())
		{
			throw std::runtime_error("Fps has not initialized.");
		}
		const auto sustain = static_cast<uint32_t>(std::floor(static_cast<float>(fp->track[6]) * ProjectParameter::fps().value() / 1000.0f));
		peak_envelope_generator.set_sustain(sustain);

		const auto release = std::ceil(static_cast<float>(fp->track[7]) * ProjectParameter::fps().value() / 1000.0f);
		peak_envelope_generator.set_release(release);
		const auto gain_val = normalize_y(fp->track[8]);
		update_gain(gain_val);
	}

	const std::function<float(float)> Limiter::effect() const noexcept
	{
		return [&](float y) -> float { return limit(scale_and_gain(y)); };
	}

	const void Limiter::fetch_trackbar_and_buffer(const AviUtl::FilterPlugin* const fp, const Buffer& buffer)
	{
		const auto orig_top = buffer.maximum();
		const auto orig_bottom = buffer.minimum();

		const auto top_limit = normalize_y(fp->track[1]);
		const auto top_threshold_diff = normalize_y(fp->track[2]);
		const auto bottom_limit = normalize_y(fp->track[3]);
		const auto bottom_threshold_diff = normalize_y(fp->track[4]);
		const auto top_diff = normalize_y(fp->track[9]);
		const auto bottom_diff = normalize_y(fp->track[10]);

		update_scale(orig_top, orig_bottom, top_diff, bottom_diff);
		const auto gained_top = scale_and_gain(orig_top);
		const auto gained_bottom = scale_and_gain(orig_bottom);
		const auto [enveloped_top, enveloped_bottom] =
			peak_envelope_generator.update_and_get_envelope_peaks(gained_top, gained_bottom);

		const auto limit_character_interpolation_mode = static_cast<InterpolationMode>(fp->track[5]);
		update_limiter(
			top_limit, top_threshold_diff,
			bottom_limit, bottom_threshold_diff,
			enveloped_top, enveloped_bottom,
			limit_character_interpolation_mode);
	}

	const void Limiter::update_from_trackbar(const AviUtl::FilterPlugin* const fp, const uint32_t track) noexcept
	{
		switch (track)
		{
		case 1U:
		{
			break;
		}
		case 2U:
		{
			const auto top_limit = normalize_y(fp->track[1]);
			const auto bottom_limit = normalize_y(fp->track[3]);
			peak_envelope_generator.set_limit(top_limit, bottom_limit);
			break;
		}
		case 4U:
		{
			const auto top_limit = normalize_y(fp->track[1]);
			const auto bottom_limit = normalize_y(fp->track[3]);
			peak_envelope_generator.set_limit(top_limit, bottom_limit);
			break;
		}
		case 7U:
		{
			const auto sustain = static_cast<uint32_t>(fp->track[6]);
			peak_envelope_generator.set_sustain(sustain);
			break;
		}
		case 8U:
		{
			const auto release = static_cast<uint32_t>(fp->track[7]);
			peak_envelope_generator.set_release(release);
			break;
		}
		case 9U:
		{
			const auto gain_val = normalize_y(fp->track[8]);
			update_gain(gain_val);
			break;
		}
		default:
			break;
		}
	}

	const void Limiter::used() noexcept
	{
		use = true;
	}

	const void Limiter::reset() noexcept
	{
		use = false;
	}

	const bool Limiter::is_using() const noexcept
	{
		return use;
	}

	BOOL Limiter::update_scale(
		const NormalizedY orig_top, const NormalizedY orig_bottom,
		const NormalizedY top_diff, const NormalizedY bottom_diff)
	{
		this->scale = make_scale(orig_top, orig_bottom, top_diff, bottom_diff);
		return true;
	}

	BOOL Limiter::update_gain(const NormalizedY gain)
	{
		this->gain = make_gain(gain);
		return true;
	}

	BOOL Limiter::update_limiter(
		const NormalizedY top_limit, const NormalizedY top_threshold_diff,
		const NormalizedY bottom_limit, const NormalizedY bottom_threshold_diff,
		const NormalizedY top_peak, const NormalizedY bottom_peak,
		InterpolationMode mode)
	{
		this->limiter = make_limit(
			make_character(
				top_limit, top_threshold_diff,
				bottom_limit, bottom_threshold_diff,
				top_peak, bottom_peak,
				select_character(mode)),
			top_limit, bottom_limit);

		return true;
	}

	NormalizedY Limiter::scale_and_gain(const NormalizedY y) const
	{
		return this->gain(this->scale(y));
	}

	NormalizedY Limiter::limit(const NormalizedY y) const
	{
		return this->limiter(y);
	}
}
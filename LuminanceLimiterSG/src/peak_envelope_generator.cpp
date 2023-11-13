/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once

#include <cmath>

#include "peak_envelope_generator.h"


namespace luminance_limiter_sg {
	BOOL PeakEnvelopeGenerator::set_limit(const double top, const double bottom) noexcept
	{
		top_limit = top;
		bottom_limit = bottom;
		return true;
	};

	BOOL PeakEnvelopeGenerator::set_sustain(const uint32_t sustain)
	{
		this->sustain = sustain;

		if (sustain == 0u)
		{
			this->top_peak_lifetime_buffer = std::nullopt;
			this->bottom_peak_lifetime_buffer = std::nullopt;
			this->active_top_peak_buffer = std::nullopt;
			this->active_bottom_peak_buffer = std::nullopt;
			return false;
		}

		const auto current_buffer_size = sustain + 1u;
		if (!top_peak_lifetime_buffer.has_value() || !active_top_peak_buffer.has_value()
			|| !bottom_peak_lifetime_buffer.has_value() || !active_bottom_peak_buffer.has_value())
		{
			this->top_peak_lifetime_buffer =
				std::make_optional<std::deque<double>>(current_buffer_size, -std::numeric_limits<double>::infinity());
			this->active_top_peak_buffer = std::make_optional<std::deque<double>>();
			this->bottom_peak_lifetime_buffer =
				std::make_optional<std::deque<double>>(current_buffer_size, std::numeric_limits<double>::infinity());
			this->active_bottom_peak_buffer = std::make_optional<std::deque<double>>();

			return true;
		}

		if (top_peak_lifetime_buffer.value().size() == current_buffer_size
			&& bottom_peak_lifetime_buffer.value().size() == current_buffer_size)
		{
			return true;
		}

		if (top_peak_lifetime_buffer.value().size() > current_buffer_size)
		{
			top_peak_lifetime_buffer.value().erase(
				top_peak_lifetime_buffer.value().begin(),
				top_peak_lifetime_buffer.value().begin()
				+ (top_peak_lifetime_buffer.value().size() - current_buffer_size));
		}

		if (bottom_peak_lifetime_buffer.value().size() > current_buffer_size)
		{
			bottom_peak_lifetime_buffer.value().erase(
				bottom_peak_lifetime_buffer.value().begin(),
				bottom_peak_lifetime_buffer.value().begin()
				+ (bottom_peak_lifetime_buffer.value().size() - current_buffer_size));
		}

		if (top_peak_lifetime_buffer.value().size() < current_buffer_size)
		{
			const auto top_shrink_size = current_buffer_size - top_peak_lifetime_buffer.value().size();
			for (auto i = 0; i < top_shrink_size; i++)
			{
				top_peak_lifetime_buffer.value().push_front(0.0);
			}
		}

		if (bottom_peak_lifetime_buffer.value().size() < current_buffer_size)
		{
			const auto bottom_shrink_size = current_buffer_size - bottom_peak_lifetime_buffer.value().size();
			for (auto i = 0; i < bottom_shrink_size; i++)
			{
				bottom_peak_lifetime_buffer.value().push_front(0.0);
			}
		}

		return true;
	};

	BOOL PeakEnvelopeGenerator::set_release(const double release)
	{
		this->release = release;
		return true;
	};

	std::array<double, 2> PeakEnvelopeGenerator::hold_peaks(const double top_peak, const double bottom_peak) noexcept {
		if (!top_peak_lifetime_buffer.has_value() || !active_top_peak_buffer.has_value()
			|| !bottom_peak_lifetime_buffer.has_value() || !active_bottom_peak_buffer.has_value())
		{
			return { top_peak, bottom_peak };
		}

		while (!active_top_peak_buffer.value().empty()
			&& active_top_peak_buffer.value().back() < top_peak)
		{
			active_top_peak_buffer.value().pop_back();
		}

		while (!active_bottom_peak_buffer.value().empty()
			&& active_bottom_peak_buffer.value().back() > bottom_peak)
		{
			active_bottom_peak_buffer.value().pop_back();
		}

		active_top_peak_buffer.value().push_back(top_peak);
		active_bottom_peak_buffer.value().push_back(bottom_peak);

		top_peak_lifetime_buffer.value().push_back(top_peak);
		const auto lifetimed_top_peak = top_peak_lifetime_buffer.value().front();
		top_peak_lifetime_buffer.value().pop_front();
		bottom_peak_lifetime_buffer.value().push_back(bottom_peak);
		const auto lifetimed_bottom_peak = bottom_peak_lifetime_buffer.value().front();
		bottom_peak_lifetime_buffer.value().pop_front();

		if (active_top_peak_buffer.value().front() == lifetimed_top_peak)
		{
			active_top_peak_buffer.value().pop_front();
		}
		const auto current_top_peak = active_top_peak_buffer.value().empty() ? top_peak : active_top_peak_buffer.value().front();

		if (active_bottom_peak_buffer.value().front() == lifetimed_bottom_peak)
		{
			active_bottom_peak_buffer.value().pop_front();
		}
		const auto current_bottom_peak = active_bottom_peak_buffer.value().empty() ? bottom_peak : active_bottom_peak_buffer.value().front();

		return { current_top_peak, current_bottom_peak };
	}

	std::array<double, 2> PeakEnvelopeGenerator::wrap_peaks(const double top_peak, const double bottom_peak) noexcept {
		double wraped_top;
		if (ongoing_top_peak == top_peak)
		{
			wraped_top = top_peak;
			top_peak_duration = 0.0;
		}
		else
		{
			top_peak_duration++;
			const auto coefficient = -(ongoing_top_peak - bottom_limit) / release;
			const auto slice = ongoing_top_peak;
			const auto released = coefficient * top_peak_duration + slice;
			if (top_peak >= released)
			{
				wraped_top = top_peak;
				ongoing_top_peak = top_peak;
				top_peak_duration = 0.0;
			}
			else
			{
				wraped_top = released;
			}
		}

		double wraped_bottom;
		if (ongoing_bottom_peak == bottom_peak)
		{
			wraped_bottom = bottom_peak;
			bottom_peak_duration = 0.0;
		}
		else
		{
			bottom_peak_duration++;
			const auto coefficient = -(ongoing_bottom_peak - top_limit) / release;
			const auto slice = ongoing_bottom_peak;
			const auto released = coefficient * bottom_peak_duration + slice;
			if (bottom_peak <= released)
			{
				wraped_bottom = bottom_peak;
				ongoing_bottom_peak = bottom_peak;
				bottom_peak_duration = 0.0;
			}
			else
			{
				wraped_bottom = released;
			}
		}
		return { wraped_top, wraped_bottom };
	};

	std::array<double, 2> PeakEnvelopeGenerator::update_and_get_envelope_peaks(const double top_peak, const double bottom_peak) noexcept {
		const auto [current_top_peak, current_bottom_peak] = hold_peaks(top_peak, bottom_peak);
		const auto [wrapped_top_peak, wrapped_bottom_peak] = wrap_peaks(current_top_peak, current_bottom_peak);
		return { wrapped_top_peak, wrapped_bottom_peak };
	};
}
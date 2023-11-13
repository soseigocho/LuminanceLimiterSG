/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "luminance_limiter_sg.h"

#include <array>
#include <deque>
#include <optional>

#include "buffer.h"

namespace luminance_limiter_sg {
	class PeakEnvelopeGenerator {
	public:
		BOOL set_limit(const double top, const double bottom) noexcept;
		BOOL set_sustain(const uint32_t sustain);
		BOOL set_release(const double release);
		std::array<double, 2> hold_peaks(const double top_peak, const double bottom_peak) noexcept;
		std::array<double, 2> wrap_peaks(const double top_peak, const double bottom_peak) noexcept;
		std::array<double, 2> update_and_get_envelope_peaks(const double top_peak, const double bottom_peak) noexcept;

	private:
		std::optional<std::deque<double>> top_peak_lifetime_buffer = std::nullopt;
		std::optional<std::deque<double>> bottom_peak_lifetime_buffer = std::nullopt;
		std::optional<std::deque<double>> active_top_peak_buffer = std::nullopt;
		std::optional<std::deque<double>> active_bottom_peak_buffer = std::nullopt;
		uint32_t sustain = 0u;
		double release = 0.0;
		double ongoing_top_peak = 0.0;
		double top_peak_duration = 0.0;
		double ongoing_bottom_peak = 0.0;
		double bottom_peak_duration = 0.0;

		double top_limit = 0.0;
		double bottom_limit = 0.0;
	};
}
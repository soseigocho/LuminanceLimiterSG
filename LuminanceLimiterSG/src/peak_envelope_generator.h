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


namespace luminance_limiter_sg {
	class PeakEnvelopeGenerator {
	public:
		BOOL set_limit(const NormalizedY top, const NormalizedY bottom) noexcept;
		BOOL set_sustain(const uint32_t sustain);
		BOOL set_release(const uint32_t release);
		std::array<NormalizedY, 2> hold_peaks(const NormalizedY top_peak, const NormalizedY bottom_peak) noexcept;
		std::array<NormalizedY, 2> wrap_peaks(const NormalizedY top_peak, const NormalizedY bottom_peak) noexcept;
		std::array<NormalizedY, 2> update_and_get_envelope_peaks(const NormalizedY top_peak, const NormalizedY bottom_peak) noexcept;

	private:
		std::optional<std::deque<NormalizedY>> top_peak_lifetime_buffer = std::nullopt;
		std::optional<std::deque<NormalizedY>> bottom_peak_lifetime_buffer = std::nullopt;
		std::optional<std::deque<NormalizedY>> active_top_peak_buffer = std::nullopt;
		std::optional<std::deque<NormalizedY>> active_bottom_peak_buffer = std::nullopt;
		uint32_t sustain = 0u;
		uint32_t release = 0u;
		NormalizedY ongoing_top_peak = 0.0f;
		uint32_t top_peak_duration = 0u;
		NormalizedY ongoing_bottom_peak = 0.0f;
		uint32_t bottom_peak_duration = 0u;

		NormalizedY top_limit = 0.0f;
		NormalizedY bottom_limit = 0.0f;
	};
}
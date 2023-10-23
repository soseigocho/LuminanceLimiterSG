/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once

#include "aviutl/filter.hpp"
#include "limiter.h"
#include "peak_envelope_generator.h"


namespace luminance_limiter_sg
{
	class RackUnit {
	public:
		RackUnit() = delete;
		RackUnit(AviUtl::FilterPlugin* fp);

		const std::function<float(float)> effect() const noexcept;
		const void fetch_trackbar_and_buffer(AviUtl::FilterPlugin* fp, const Buffer& buffer);

		const void used() noexcept;
		const void reset() noexcept;

		const bool is_using() const noexcept;

		Limiter limiter;
		PeakEnvelopeGenerator peak_envelope_generator;
	private:
		bool use = false;
	};
}
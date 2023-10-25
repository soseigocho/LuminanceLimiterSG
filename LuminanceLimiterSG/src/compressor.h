/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once

#include <functional>

#include "amplifier.h"
#include "rack_unit.h"

namespace luminance_limiter_sg
{
	class Compressor final : public IRackUnit
	{
	public:
		Compressor(const AviUtl::FilterPlugin* const fp);

		const std::function<float(float)> effect() const noexcept;
		const void fetch_trackbar_and_buffer(const AviUtl::FilterPlugin* const fp, const Buffer& buffer) override;
		const void update_from_trackbar(const AviUtl::FilterPlugin* const fp, const uint32_t track) override;

		const void used() noexcept override;
		const void reset() noexcept override;

		const bool is_using() const noexcept override;
	private:
		bool use = false;

		Amplifier pre, post;
	};
}
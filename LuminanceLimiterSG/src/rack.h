/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#pragma once

#include "rack_unit.h"

#include <array>
#include <memory>

#include "processing_mode.h"


namespace luminance_limiter_sg
{
	constexpr static inline auto num_or_racks = 16U;

	class Rack
	{
	public:
		Rack();

		const void gc() noexcept;

		const bool is_first_time(uint32_t current_frame) noexcept;
		const void set_effector(uint32_t idx, ProcessingMode processing_mode, AviUtl::FilterPlugin* fp);

		uint32_t size() const noexcept;

		std::optional<std::unique_ptr<RackUnit>>& operator[] (size_t idx) noexcept;
	private:
		uint32_t ongoing_frame = 0;
		std::array<std::optional<std::unique_ptr<RackUnit>>, num_or_racks> elements;
	};
}
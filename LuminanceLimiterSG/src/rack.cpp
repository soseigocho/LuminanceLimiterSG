/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/


#include "rack.h"


namespace luminance_limiter_sg
{
	Rack::Rack()
	{
		std::fill(elements.begin(), elements.end(), std::nullopt);
	}

	const void Rack::gc() noexcept
	{
		for (auto&& elem: elements)
		{
			if (elem)
			{
				if (!elem->is_using())
				{
					elem.reset();
				}
				else
				{
					elem->reset();
				}
			}
		}
	}

	const bool Rack::is_first_time(uint32_t current_frame) noexcept
	{
		const auto result = ongoing_frame < current_frame;
		if (result)
		{
			ongoing_frame = current_frame;
		}
		return result;
	}

	const void Rack::set_effector(uint32_t idx, const AviUtl::FilterPlugin* const fp)
	{
		elements[idx].emplace(Limiter(fp));
	}

	uint32_t Rack::size() const noexcept
	{
		return elements.size();
	}

	std::optional<Limiter>& Rack::operator[] (size_t idx) noexcept
	{
		return elements[idx];
	}
}
/*
	Copyright(c) 2023 SoseiGocho
	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0.If a copy of the MPL was not distributed with this file, You can
	obtain one at https ://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "limiter.h"
#include "peak_envelope_generator.h"

namespace luminance_limiter_sg {
	struct RuckUnit {
		bool used;
		Limiter limiter;
		PeakEnvelopeGenerator peak_envelope_generator;
	};
}
#pragma once

#include "limiter.h"
#include "peak_envelope_generator.h"

namespace luminance_limiter_sg {
	struct RuckUnit {
		Limiter limiter;
		PeakEnvelopeGenerator peak_envelope_generator;
	};
}
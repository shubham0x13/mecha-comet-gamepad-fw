/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdint.h>

#include "util/stick_calibration.h"

/* The range is made this much smaller, in percent, so the stick always reaches the ends. */
#define STICK_OUTER_MARGIN_PERCENT 3

/*
 * A usable range keeps its centre within half of the reference half-range from the reference
 * centre, and its half-range is between half and twice the reference half-range. Anything else is
 * a failed measurement, for example a stick that was not turned all the way round.
 */
bool stick_calibration_is_valid(const struct stick_range *range,
				const struct stick_range *reference)
{
	int32_t ref_half = ((int32_t)reference->in_max - reference->in_min) / 2;
	int32_t ref_mid = ((int32_t)reference->in_max + reference->in_min) / 2;
	int32_t half = ((int32_t)range->in_max - range->in_min) / 2;
	int32_t mid = ((int32_t)range->in_max + range->in_min) / 2;
	int32_t offset = (mid > ref_mid) ? (mid - ref_mid) : (ref_mid - mid);

	if (ref_half <= 0 || range->in_min >= range->in_max) {
		return false;
	}

	return offset <= ref_half / 2 && half >= ref_half / 2 && half <= ref_half * 2;
}

bool stick_calibration_compute(int16_t centre, int16_t lowest, int16_t highest,
			       const struct stick_range *reference, struct stick_range *out)
{
	struct stick_range range;
	int32_t half;

	if (lowest >= centre || highest <= centre) {
		return false;
	}

	half = ((centre - lowest) < (highest - centre)) ? (centre - lowest) : (highest - centre);
	half -= half * STICK_OUTER_MARGIN_PERCENT / 100;

	range.in_min = (int16_t)(centre - half);
	range.in_max = (int16_t)(centre + half);

	if (!stick_calibration_is_valid(&range, reference)) {
		return false;
	}

	*out = range;

	return true;
}

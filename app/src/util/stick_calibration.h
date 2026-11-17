/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_STICK_CALIBRATION_H_
#define APP_STICK_CALIBRATION_H_

#include <stdbool.h>
#include <stdint.h>

/** Raw ADC values that are mapped to the ends of an axis. */
struct stick_range {
	int16_t in_min;
	int16_t in_max;
};

/**
 * Make the range of an axis from a measurement.
 *
 * The analog-axis driver takes the middle of in_min and in_max as the centre of the stick, so
 * the range is made symmetric around the measured rest position. It uses the shorter side, made a
 * little smaller, so that the stick reaches the end of the axis on both sides.
 *
 * @param centre Raw value with the stick at rest.
 * @param lowest Lowest raw value seen while turning the stick.
 * @param highest Highest raw value seen while turning the stick.
 * @param reference Range from the devicetree, used to reject impossible measurements.
 * @param out Receives the new range.
 *
 * @retval true if the measurement gives a usable range.
 * @retval false if it does not; out is not changed then.
 */
bool stick_calibration_compute(int16_t centre, int16_t lowest, int16_t highest,
			       const struct stick_range *reference, struct stick_range *out);

/**
 * Check that a range, such as one read from flash, is usable.
 *
 * @param range The range to check.
 * @param reference Range from the devicetree.
 *
 * @retval true if the range is usable.
 */
bool stick_calibration_is_valid(const struct stick_range *range,
				const struct stick_range *reference);

#endif /* APP_STICK_CALIBRATION_H_ */

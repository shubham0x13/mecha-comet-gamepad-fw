/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_LEFT_STICK_H_
#define APP_LEFT_STICK_H_

/**
 * Start the calibration of the left stick.
 *
 * The calibration goes through these steps, each shown on the status LED (see gamepad_events.h):
 *
 * 1. Release all buttons and the stick. Its rest position is measured.
 * 2. Turn the stick all the way round a few times. Its range is measured.
 * 3. Press A (south) to save, or B (east) to cancel.
 *
 * The result is stored in flash and used at every start. Nothing changes when the calibration is
 * cancelled, times out or gives an impossible range. It does nothing when the left stick is not in
 * the devicetree or is disabled.
 */
void left_stick_calibration_start(void);

#endif /* APP_LEFT_STICK_H_ */

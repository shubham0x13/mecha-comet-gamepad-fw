/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_STATUS_LED_H_
#define APP_STATUS_LED_H_

/**
 * Start showing the state of the gamepad on the status LED.
 *
 * The LED follows the USB state and shows the gamepad events (see gamepad_events.h):
 *
 * - Slow blink: waiting for the host.
 * - On: connected and set up by the host, also while the host sleeps.
 * - Two short blinks: the right stick was recalibrated.
 * - Fast blink: left stick calibration, release the stick.
 * - Double blink: left stick calibration, turn the stick all the way round.
 * - Three short blinks: calibration saved. One long blink: calibration failed.
 * - Very fast blink: factory reset or restart into the boot ROM.
 *
 * @retval 0 on success.
 * @retval -ENODEV if the LED is not ready.
 */
#ifdef CONFIG_COMET_GAMEPAD_STATUS_LED
int status_led_init(void);
#else
static inline int status_led_init(void)
{
	return 0;
}
#endif

#endif /* APP_STATUS_LED_H_ */

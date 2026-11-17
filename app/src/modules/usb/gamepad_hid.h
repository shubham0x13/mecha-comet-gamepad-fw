/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_GAMEPAD_HID_H_
#define APP_GAMEPAD_HID_H_

#include <stdint.h>

#include <zephyr/kernel.h>

#include "modules/gamepad_state.h"

/**
 * Register the gamepad with the USB HID class.
 *
 * Call this before gamepad_usb_init().
 *
 * @param ready Semaphore given when the host is ready to receive reports.
 *
 * @retval 0 on success.
 * @retval -ENODEV if the HID device is not ready.
 */
int gamepad_hid_init(struct k_sem *ready);

/**
 * Send the gamepad state to the host as an HID input report.
 *
 * @param state State to send.
 *
 * @retval 0 on success.
 * @retval -EAGAIN if the host is not ready for reports yet.
 */
int gamepad_hid_submit(const struct gamepad_state *state);

/**
 * Get the idle rate that the host set with the HID Set_Idle request.
 *
 * When it is not 0, the host wants the current report again after this time, even if nothing
 * changed.
 *
 * @return Idle rate in milliseconds, 0 to send reports only on change.
 */
uint32_t gamepad_hid_idle_ms(void);

#endif /* APP_GAMEPAD_HID_H_ */

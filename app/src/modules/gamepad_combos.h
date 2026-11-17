/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_GAMEPAD_COMBOS_H_
#define APP_GAMEPAD_COMBOS_H_

/**
 * Set up the button combinations of the gamepad.
 *
 * @retval 0 on success.
 * @retval negative errno on failure.
 */
int gamepad_combos_init(void);

#endif /* APP_GAMEPAD_COMBOS_H_ */

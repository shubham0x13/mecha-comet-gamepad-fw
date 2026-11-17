/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_FACTORY_RESET_H_
#define APP_FACTORY_RESET_H_

/**
 * Erase everything that is stored in flash, so that the gamepad uses its default settings, and
 * restart.
 *
 * Only the storage partition is erased, never the firmware. This function does not return.
 */
void factory_reset_perform(void);

#endif /* APP_FACTORY_RESET_H_ */

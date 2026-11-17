/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_BOOTLOADER_H_
#define APP_BOOTLOADER_H_

/**
 * Restart the chip into its boot ROM, so that new firmware can be written over USB.
 *
 * It works as if the BOOT pin (PTA4) were held low at the reset. Nothing is written to flash, so
 * a reset or a power cycle starts the firmware again. This function does not return.
 */
void bootloader_enter(void);

#endif /* APP_BOOTLOADER_H_ */

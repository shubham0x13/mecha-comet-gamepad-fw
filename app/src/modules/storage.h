/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_STORAGE_H_
#define APP_STORAGE_H_

#include <stddef.h>

/**
 * Settings that are kept in flash, with the Zephyr settings subsystem (NVS backend).
 *
 * Each part of the firmware owns its own key under "gamepad/" and loads its value with
 * SETTINGS_STATIC_HANDLER_DEFINE(). A value starts with a version number, so that a newer
 * firmware can read or ignore what an older one stored. A missing or invalid value means the
 * default from the devicetree is used.
 */

/**
 * Start the settings subsystem and load every stored value.
 *
 * @retval 0 on success.
 * @retval negative errno on failure. The defaults are used then.
 */
int storage_init(void);

/**
 * Store a value.
 *
 * @param key Full key, for example "gamepad/left_stick".
 * @param value The value.
 * @param len Size of the value in bytes.
 *
 * @retval 0 on success.
 * @retval negative errno on failure.
 */
int storage_save(const char *key, const void *value, size_t len);

/**
 * Erase everything that is stored, so that every part uses its default after the next start.
 *
 * Only the storage partition is erased, never the firmware. Restart after this, because the
 * settings subsystem does not know that its data is gone.
 *
 * @retval 0 on success.
 * @retval negative errno on failure.
 */
int storage_erase_all(void);

#endif /* APP_STORAGE_H_ */

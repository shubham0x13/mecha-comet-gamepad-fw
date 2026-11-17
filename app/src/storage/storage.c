/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/storage/flash_map.h>

#include "storage/storage.h"

LOG_MODULE_REGISTER(storage, CONFIG_COMET_GAMEPAD_LOG_LEVEL);

int storage_init(void)
{
	int ret;

	ret = settings_subsys_init();
	if (ret < 0) {
		LOG_ERR("Failed to start the settings subsystem (%d)", ret);
		return ret;
	}

	ret = settings_load();
	if (ret < 0) {
		LOG_ERR("Failed to load the settings (%d)", ret);
		return ret;
	}

	return 0;
}

int storage_save(const char *key, const void *value, size_t len)
{
	int ret = settings_save_one(key, value, len);

	if (ret < 0) {
		LOG_ERR("Failed to store %s (%d)", key, ret);
	}

	return ret;
}

int storage_erase_all(void)
{
	const struct flash_area *area;
	int ret;

	ret = flash_area_open(PARTITION_ID(storage_partition), &area);
	if (ret < 0) {
		LOG_ERR("Failed to open the storage partition (%d)", ret);
		return ret;
	}

	ret = flash_area_erase(area, 0, area->fa_size);
	flash_area_close(area);
	if (ret < 0) {
		LOG_ERR("Failed to erase the storage partition (%d)", ret);
	}

	return ret;
}

/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

#include "events/gamepad_events.h"
#include "factory_reset/factory_reset.h"
#include "storage/storage.h"

LOG_MODULE_REGISTER(factory_reset, CONFIG_COMET_GAMEPAD_LOG_LEVEL);

/* Time to show the event on the status LED before the restart. */
#define FACTORY_RESET_NOTICE_MS 1000

void factory_reset_perform(void)
{
	/* Nothing is shown when the erase fails, so the LED does not stay in this pattern. */
	if (storage_erase_all() < 0) {
		return;
	}

	LOG_WRN("Settings erased, restarting");
	gamepad_event_publish(GAMEPAD_EVENT_FACTORY_RESET);
	k_msleep(FACTORY_RESET_NOTICE_MS);

	/* The restart also shows on the host as the gamepad being unplugged and plugged in. */
	sys_reboot(SYS_REBOOT_COLD);
}

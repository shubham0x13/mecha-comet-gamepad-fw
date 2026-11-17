/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <soc.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

#include "modules/bootloader.h"
#include "events/gamepad_events.h"

LOG_MODULE_REGISTER(bootloader, CONFIG_COMET_GAMEPAD_LOG_LEVEL);

/* Time to show the event on the status LED before the restart. */
#define BOOTLOADER_NOTICE_MS 1000

void bootloader_enter(void)
{
	gamepad_event_publish(GAMEPAD_EVENT_BOOTLOADER);

	LOG_WRN("Restarting into the boot ROM");
	k_msleep(BOOTLOADER_NOTICE_MS);

	/* The reset control module boots from the ROM at the next reset. */
	RCM->FM = RCM_FM_FORCEROM(1);

	sys_reboot(SYS_REBOOT_COLD);
}

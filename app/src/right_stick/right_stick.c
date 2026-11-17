/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#include <mecha/input/pivot_pip.h>

#include "events/gamepad_events.h"
#include "right_stick/right_stick.h"

LOG_MODULE_REGISTER(right_stick, CONFIG_COMET_GAMEPAD_LOG_LEVEL);

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pivot_pip))

void right_stick_recalibrate(void)
{
	int ret = pivot_pip_recalibrate(DEVICE_DT_GET(DT_NODELABEL(pivot_pip)));

	if (ret < 0) {
		LOG_ERR("Failed to recalibrate the right stick (%d)", ret);
		return;
	}

	gamepad_event_publish(GAMEPAD_EVENT_RIGHT_STICK_RECALIBRATED);
}

#else

void right_stick_recalibrate(void)
{
	LOG_WRN("There is no right stick");
}

#endif /* DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pivot_pip)) */

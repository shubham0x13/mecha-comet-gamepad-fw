/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Power saving while the host has suspended USB, for example while it sleeps: the sticks stop
 * sampling, and they start again when the host wakes the bus up. The buttons stay on.
 */

#include <errno.h>
#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/zbus.h>

#include "events/gamepad_events.h"

LOG_MODULE_REGISTER(power_saving, CONFIG_COMET_GAMEPAD_LOG_LEVEL);

#define LEFT_STICK_NODE  DT_NODELABEL(left_stick)
#define RIGHT_STICK_NODE DT_NODELABEL(pivot_pip)

/* The devices that stop while the host sleeps. */
static const struct device *const sticks[] = {
#if DT_NODE_HAS_STATUS_OKAY(LEFT_STICK_NODE)
	DEVICE_DT_GET(LEFT_STICK_NODE),
#endif
#if DT_NODE_HAS_STATUS_OKAY(RIGHT_STICK_NODE)
	DEVICE_DT_GET(RIGHT_STICK_NODE),
#endif
};

/* Only used in the listener, which runs in the thread that publishes the USB state. */
static bool suspended;

static void power_saving_set(enum pm_device_action action)
{
	for (size_t i = 0; i < ARRAY_SIZE(sticks); i++) {
		int ret = pm_device_action_run(sticks[i], action);

		if (ret < 0 && ret != -EALREADY) {
			LOG_WRN("Failed to change the power state of %s (%d)", sticks[i]->name,
				ret);
		}
	}
}

static void power_saving_listener_cb(const struct zbus_channel *chan)
{
	const struct usb_state_msg *msg = zbus_chan_const_msg(chan);
	bool suspend = (msg->state == USB_STATE_SUSPENDED);

	if (suspend == suspended) {
		return;
	}

	suspended = suspend;
	power_saving_set(suspend ? PM_DEVICE_ACTION_SUSPEND : PM_DEVICE_ACTION_RESUME);
	LOG_INF("Sticks %s", suspend ? "suspended" : "resumed");
}

ZBUS_LISTENER_DEFINE(power_saving_listener, power_saving_listener_cb);
ZBUS_CHAN_ADD_OBS(usb_state_chan, power_saving_listener, 0);

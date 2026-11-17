/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "events/gamepad_events.h"

LOG_MODULE_REGISTER(gamepad_events, CONFIG_COMET_GAMEPAD_LOG_LEVEL);

/* Observers add themselves with ZBUS_CHAN_ADD_OBS(), so this file does not know them. */
ZBUS_CHAN_DEFINE(usb_state_chan, struct usb_state_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(.state = USB_STATE_DISCONNECTED));

ZBUS_CHAN_DEFINE(gamepad_event_chan, struct gamepad_event_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

void gamepad_event_publish(enum gamepad_event event)
{
	const struct gamepad_event_msg msg = {.event = event};
	int ret;

	ret = zbus_chan_pub(&gamepad_event_chan, &msg, K_MSEC(100));
	if (ret < 0) {
		LOG_WRN("Failed to publish event %d (%d)", event, ret);
	}
}

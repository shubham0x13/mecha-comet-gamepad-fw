/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_GAMEPAD_EVENTS_H_
#define APP_GAMEPAD_EVENTS_H_

#include <zephyr/zbus/zbus.h>

/**
 * The state of the USB connection. Published on usb_state_chan by the USB part, read by
 * everything that shows or depends on it, such as the status LED.
 */
enum usb_state {
	/** There is no USB connection, or it is being set up. */
	USB_STATE_DISCONNECTED,
	/** The host found the gamepad but has not set it up yet. */
	USB_STATE_CONNECTED,
	/** The host set up the gamepad and reads its reports. */
	USB_STATE_CONFIGURED,
	/** The host suspended the bus, for example when it sleeps. */
	USB_STATE_SUSPENDED,
};

struct usb_state_msg {
	enum usb_state state;
};

/**
 * Things that happen on the gamepad and that the user should see. Published on gamepad_event_chan
 * by the part where they happen.
 */
enum gamepad_event {
	/** The rest position of the right stick was measured again. */
	GAMEPAD_EVENT_RIGHT_STICK_RECALIBRATED,
	/** Left stick calibration started: the stick must be released. */
	GAMEPAD_EVENT_CALIBRATION_CENTRE,
	/** Left stick calibration measures the range: the stick must be turned all the way round.
	 */
	GAMEPAD_EVENT_CALIBRATION_RANGE,
	/** Left stick calibration finished and was saved. */
	GAMEPAD_EVENT_CALIBRATION_SAVED,
	/** Left stick calibration was cancelled or failed. Nothing was changed. */
	GAMEPAD_EVENT_CALIBRATION_FAILED,
	/** The stored settings are being erased, then the gamepad restarts. */
	GAMEPAD_EVENT_FACTORY_RESET,
	/** The gamepad restarts into the boot ROM to receive new firmware. */
	GAMEPAD_EVENT_BOOTLOADER,
};

struct gamepad_event_msg {
	enum gamepad_event event;
};

ZBUS_CHAN_DECLARE(usb_state_chan, gamepad_event_chan);

/**
 * Publish a gamepad event.
 *
 * @param event The event.
 */
void gamepad_event_publish(enum gamepad_event event);

#endif /* APP_GAMEPAD_EVENTS_H_ */

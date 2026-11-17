/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/zbus.h>

#include "events/gamepad_events.h"
#include "status_led/status_led.h"

LOG_MODULE_REGISTER(status_led, CONFIG_COMET_GAMEPAD_LOG_LEVEL);

/* The LED is on or off for ms milliseconds. A duration of 0 keeps it like that. */
struct led_segment {
	bool on;
	uint16_t ms;
};

struct led_pattern {
	const struct led_segment *segments;
	uint8_t num_segments;
	/* A pattern that does not repeat returns to the base pattern when it ends. */
	bool repeat;
};

#define LED_PATTERN(_repeat, ...)                                                                  \
	{                                                                                          \
		.segments = (const struct led_segment[]){__VA_ARGS__},                             \
		.num_segments = sizeof((const struct led_segment[]){__VA_ARGS__}) /                \
				sizeof(struct led_segment),                                        \
		.repeat = _repeat,                                                                 \
	}

/* Base patterns, from the USB state. */
static const struct led_pattern pattern_on = LED_PATTERN(false, {true, 0});
static const struct led_pattern pattern_waiting = LED_PATTERN(true, {true, 500}, {false, 500});

/* Event patterns, shown over the base pattern. */
static const struct led_pattern pattern_two_blinks =
	LED_PATTERN(false, {false, 150}, {true, 150}, {false, 150}, {true, 150}, {false, 300});
static const struct led_pattern pattern_three_blinks =
	LED_PATTERN(false, {false, 150}, {true, 150}, {false, 150}, {true, 150}, {false, 150},
		    {true, 150}, {false, 300});
static const struct led_pattern pattern_long_blink =
	LED_PATTERN(false, {false, 300}, {true, 1500}, {false, 300});
static const struct led_pattern pattern_fast = LED_PATTERN(true, {true, 100}, {false, 100});
static const struct led_pattern pattern_double =
	LED_PATTERN(true, {true, 100}, {false, 100}, {true, 100}, {false, 700});
static const struct led_pattern pattern_very_fast = LED_PATTERN(true, {true, 50}, {false, 50});

static const struct led_dt_spec led = LED_DT_SPEC_GET(DT_NODELABEL(status_led));

static void status_led_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(led_work, status_led_work_handler);

static struct k_spinlock lock;
static const struct led_pattern *base = &pattern_waiting;
static const struct led_pattern *overlay;
static size_t step;
static atomic_t ready;

static void status_led_work_handler(struct k_work *work)
{
	const struct led_pattern *pattern;
	struct led_segment segment;
	k_spinlock_key_t key;

	ARG_UNUSED(work);

	key = k_spin_lock(&lock);

	pattern = (overlay != NULL) ? overlay : base;
	if (step >= pattern->num_segments) {
		if (pattern == overlay && !overlay->repeat) {
			overlay = NULL;
			pattern = base;
		}
		step = 0;
	}
	segment = pattern->segments[step++];

	k_spin_unlock(&lock, key);

	if (segment.on) {
		(void)led_on_dt(&led);
	} else {
		(void)led_off_dt(&led);
	}

	if (segment.ms > 0) {
		k_work_reschedule(&led_work, K_MSEC(segment.ms));
	}
}

static void status_led_show(const struct led_pattern *new_base,
			    const struct led_pattern *new_overlay)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	bool restart = false;

	if (new_base != NULL && new_base != base) {
		base = new_base;
		/* A running event pattern finishes first. */
		restart = (overlay == NULL);
	}

	if (new_overlay != NULL) {
		overlay = new_overlay;
		restart = true;
	}

	if (restart) {
		step = 0;
	}

	k_spin_unlock(&lock, key);

	if (restart && atomic_get(&ready)) {
		k_work_reschedule(&led_work, K_NO_WAIT);
	}
}

static const struct led_pattern *status_led_usb_pattern(enum usb_state state)
{
	switch (state) {
	/*
	 * The firmware keeps running while the host sleeps, so the LED stays on to show power.
	 * Turn it off in suspend once suspend also puts the chip to sleep.
	 */
	case USB_STATE_CONFIGURED:
	case USB_STATE_SUSPENDED:
		return &pattern_on;
	case USB_STATE_DISCONNECTED:
	case USB_STATE_CONNECTED:
	default:
		return &pattern_waiting;
	}
}

static const struct led_pattern *status_led_event_pattern(enum gamepad_event event)
{
	switch (event) {
	case GAMEPAD_EVENT_RIGHT_STICK_RECALIBRATED:
		return &pattern_two_blinks;
	case GAMEPAD_EVENT_CALIBRATION_CENTRE:
		return &pattern_fast;
	case GAMEPAD_EVENT_CALIBRATION_RANGE:
		return &pattern_double;
	case GAMEPAD_EVENT_CALIBRATION_SAVED:
		return &pattern_three_blinks;
	case GAMEPAD_EVENT_CALIBRATION_FAILED:
		return &pattern_long_blink;
	case GAMEPAD_EVENT_FACTORY_RESET:
	case GAMEPAD_EVENT_BOOTLOADER:
		return &pattern_very_fast;
	default:
		return NULL;
	}
}

static void status_led_listener_cb(const struct zbus_channel *chan)
{
	if (chan == &usb_state_chan) {
		const struct usb_state_msg *msg = zbus_chan_const_msg(chan);

		status_led_show(status_led_usb_pattern(msg->state), NULL);
	} else if (chan == &gamepad_event_chan) {
		const struct gamepad_event_msg *msg = zbus_chan_const_msg(chan);

		status_led_show(NULL, status_led_event_pattern(msg->event));
	}
}

ZBUS_LISTENER_DEFINE(status_led_listener, status_led_listener_cb);
ZBUS_CHAN_ADD_OBS(usb_state_chan, status_led_listener, 0);
ZBUS_CHAN_ADD_OBS(gamepad_event_chan, status_led_listener, 0);

int status_led_init(void)
{
	if (!led_is_ready_dt(&led)) {
		LOG_ERR("The status LED is not ready");
		return -ENODEV;
	}

	atomic_set(&ready, 1);
	k_work_reschedule(&led_work, K_NO_WAIT);

	return 0;
}

/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_analog_axis.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#include "combo/combo.h"
#include "events/gamepad_events.h"
#include "left_stick/left_stick.h"
#include "left_stick/stick_calibration.h"
#include "storage/storage.h"

LOG_MODULE_REGISTER(left_stick, CONFIG_COMET_GAMEPAD_LOG_LEVEL);

#define LEFT_STICK_NODE DT_NODELABEL(left_stick)

#if DT_NODE_HAS_STATUS_OKAY(LEFT_STICK_NODE)

#define LEFT_STICK_NUM_AXES DT_CHILD_NUM_STATUS_OKAY(LEFT_STICK_NODE)

/* The calibration is stored as "left_stick/cal". */
#define SETTINGS_TREE "left_stick"
#define SETTINGS_KEY  "cal"

/*
 * Layout of the stored calibration. When it changes, increase the version and keep reading the
 * older versions, so that an update does not lose the calibration.
 */
#define RECORD_VERSION 1

struct left_stick_record {
	uint8_t version;
	uint8_t num_axes;
	struct stick_range axis[LEFT_STICK_NUM_AXES];
};

/*
 * The fields are in an order that needs no padding, so the stored bytes do not depend on the
 * compiler. It is not packed, because the Cortex-M0+ cannot read unaligned 16-bit values.
 */
BUILD_ASSERT(sizeof(struct left_stick_record) ==
		     2 + LEFT_STICK_NUM_AXES * sizeof(struct stick_range),
	     "struct left_stick_record has padding");

/* Calibration timing, in milliseconds. */
#define CAL_CHECK_PERIOD_MS    50
#define CAL_RELEASE_SETTLE_MS  1000
#define CAL_RELEASE_TIMEOUT_MS 10000
#define CAL_CENTRE_MS          500
#define CAL_RANGE_TIMEOUT_MS   30000

/* Buttons that end the range step. */
#define CAL_SAVE_CODE   INPUT_BTN_SOUTH
#define CAL_CANCEL_CODE INPUT_BTN_EAST

enum cal_step {
	CAL_IDLE,
	CAL_WAIT_RELEASE,
	CAL_CENTRE,
	CAL_RANGE,
};

struct cal_state {
	enum cal_step step;
	int64_t step_start_ms;
	/* Time since when no button is held, or -1 while one is held. */
	int64_t released_since_ms;
	int32_t sum[LEFT_STICK_NUM_AXES];
	uint32_t count[LEFT_STICK_NUM_AXES];
	int16_t centre[LEFT_STICK_NUM_AXES];
	int16_t lowest[LEFT_STICK_NUM_AXES];
	int16_t highest[LEFT_STICK_NUM_AXES];
	bool save;
	bool cancel;
};

static const struct device *const stick = DEVICE_DT_GET(LEFT_STICK_NODE);

/* Ranges from the devicetree, read before anything else is applied. */
static struct stick_range defaults[LEFT_STICK_NUM_AXES];
static bool defaults_read;

static struct cal_state cal;
static struct k_spinlock cal_lock;

static void left_stick_cal_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(cal_work, left_stick_cal_work_handler);

static int left_stick_read_defaults(void)
{
	if (defaults_read) {
		return 0;
	}

	if (!device_is_ready(stick)) {
		return -ENODEV;
	}

	if (analog_axis_num_axes(stick) != LEFT_STICK_NUM_AXES) {
		return -EINVAL;
	}

	for (int i = 0; i < LEFT_STICK_NUM_AXES; i++) {
		struct analog_axis_calibration axis_cal;
		int ret = analog_axis_calibration_get(stick, i, &axis_cal);

		if (ret < 0) {
			return ret;
		}

		defaults[i].in_min = axis_cal.in_min;
		defaults[i].in_max = axis_cal.in_max;
	}

	defaults_read = true;

	return 0;
}

/* Set new ranges. The deadzones from the devicetree are kept. */
static int left_stick_apply(const struct stick_range ranges[LEFT_STICK_NUM_AXES])
{
	for (int i = 0; i < LEFT_STICK_NUM_AXES; i++) {
		struct analog_axis_calibration axis_cal;
		int ret = analog_axis_calibration_get(stick, i, &axis_cal);

		if (ret < 0) {
			return ret;
		}

		axis_cal.in_min = ranges[i].in_min;
		axis_cal.in_max = ranges[i].in_max;

		ret = analog_axis_calibration_set(stick, i, &axis_cal);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static bool left_stick_record_is_valid(const struct left_stick_record *record)
{
	if (record->version != RECORD_VERSION || record->num_axes != LEFT_STICK_NUM_AXES) {
		return false;
	}

	for (int i = 0; i < LEFT_STICK_NUM_AXES; i++) {
		if (!stick_calibration_is_valid(&record->axis[i], &defaults[i])) {
			return false;
		}
	}

	return true;
}

static int left_stick_settings_set(const char *key, size_t len, settings_read_cb read_cb,
				   void *cb_arg)
{
	struct left_stick_record record;
	const char *next;
	ssize_t ret;

	if (!settings_name_steq(key, SETTINGS_KEY, &next) || next != NULL) {
		return -ENOENT;
	}

	if (left_stick_read_defaults() < 0) {
		LOG_ERR("The left stick is not ready, the stored calibration is not used");
		return 0;
	}

	if (len != sizeof(record)) {
		LOG_WRN("Ignoring a stored calibration with an unknown layout");
		return 0;
	}

	ret = read_cb(cb_arg, &record, sizeof(record));
	if (ret != sizeof(record)) {
		LOG_WRN("Failed to read the stored calibration (%d)", (int)ret);
		return 0;
	}

	if (!left_stick_record_is_valid(&record)) {
		LOG_WRN("Ignoring an invalid stored calibration");
		return 0;
	}

	ret = left_stick_apply(record.axis);
	if (ret < 0) {
		LOG_ERR("Failed to apply the stored calibration (%d)", (int)ret);
		return 0;
	}

	LOG_INF("Using the stored left stick calibration");

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(left_stick, SETTINGS_TREE, NULL, left_stick_settings_set, NULL,
			       NULL);

static void left_stick_raw_cb(const struct device *dev, int channel, int16_t raw_val)
{
	k_spinlock_key_t key;

	ARG_UNUSED(dev);

	if (channel < 0 || channel >= LEFT_STICK_NUM_AXES) {
		return;
	}

	key = k_spin_lock(&cal_lock);

	if (cal.step == CAL_CENTRE) {
		cal.sum[channel] += raw_val;
		cal.count[channel]++;
	} else if (cal.step == CAL_RANGE) {
		cal.lowest[channel] = MIN(cal.lowest[channel], raw_val);
		cal.highest[channel] = MAX(cal.highest[channel], raw_val);
	}

	k_spin_unlock(&cal_lock, key);
}

static void left_stick_input_cb(struct input_event *evt, void *user_data)
{
	k_spinlock_key_t key;
	bool requested = false;

	ARG_UNUSED(user_data);

	if (evt->type != INPUT_EV_KEY || evt->value == 0) {
		return;
	}

	key = k_spin_lock(&cal_lock);

	if (cal.step == CAL_RANGE) {
		if (evt->code == CAL_SAVE_CODE) {
			cal.save = true;
			requested = true;
		} else if (evt->code == CAL_CANCEL_CODE) {
			cal.cancel = true;
			requested = true;
		}
	}

	k_spin_unlock(&cal_lock, key);

	if (requested) {
		k_work_reschedule(&cal_work, K_NO_WAIT);
	}
}

INPUT_CALLBACK_DEFINE(NULL, left_stick_input_cb, NULL);

static void left_stick_cal_end(bool saved)
{
	k_spinlock_key_t key;

	analog_axis_set_raw_data_cb(stick, NULL);

	key = k_spin_lock(&cal_lock);
	cal.step = CAL_IDLE;
	k_spin_unlock(&cal_lock, key);

	gamepad_event_publish(saved ? GAMEPAD_EVENT_CALIBRATION_SAVED
				    : GAMEPAD_EVENT_CALIBRATION_FAILED);
}

/* Store and use the measured ranges. Nothing changes if any step fails. */
static bool left_stick_cal_save(const struct cal_state *result)
{
	struct left_stick_record record = {
		.version = RECORD_VERSION,
		.num_axes = LEFT_STICK_NUM_AXES,
	};
	int ret;

	for (int i = 0; i < LEFT_STICK_NUM_AXES; i++) {
		if (!stick_calibration_compute(result->centre[i], result->lowest[i],
					       result->highest[i], &defaults[i], &record.axis[i])) {
			LOG_WRN("Axis %d: centre %d, range %d to %d is not usable", i,
				result->centre[i], result->lowest[i], result->highest[i]);
			return false;
		}
	}

	ret = storage_save(SETTINGS_TREE "/" SETTINGS_KEY, &record, sizeof(record));
	if (ret < 0) {
		return false;
	}

	ret = left_stick_apply(record.axis);
	if (ret < 0) {
		LOG_ERR("Failed to apply the calibration (%d)", ret);
		return false;
	}

	LOG_INF("Left stick calibration saved");

	return true;
}

static void left_stick_cal_work_handler(struct k_work *work)
{
	int64_t now = k_uptime_get();
	enum gamepad_event event = GAMEPAD_EVENT_CALIBRATION_FAILED;
	bool publish = false;
	bool end = false;
	bool save = false;
	struct cal_state result;
	k_spinlock_key_t key;

	ARG_UNUSED(work);

	key = k_spin_lock(&cal_lock);

	switch (cal.step) {
	case CAL_WAIT_RELEASE:
		if (combo_keys_held() > 0) {
			cal.released_since_ms = -1;
			end = (now - cal.step_start_ms) >= CAL_RELEASE_TIMEOUT_MS;
		} else if (cal.released_since_ms < 0) {
			cal.released_since_ms = now;
		} else if ((now - cal.released_since_ms) >= CAL_RELEASE_SETTLE_MS) {
			for (int i = 0; i < LEFT_STICK_NUM_AXES; i++) {
				cal.sum[i] = 0;
				cal.count[i] = 0;
			}
			cal.step = CAL_CENTRE;
			cal.step_start_ms = now;
		}
		break;

	case CAL_CENTRE:
		if ((now - cal.step_start_ms) < CAL_CENTRE_MS) {
			break;
		}

		for (int i = 0; i < LEFT_STICK_NUM_AXES; i++) {
			if (cal.count[i] == 0) {
				end = true;
				break;
			}
			cal.centre[i] = (int16_t)(cal.sum[i] / (int32_t)cal.count[i]);
			cal.lowest[i] = cal.centre[i];
			cal.highest[i] = cal.centre[i];
		}

		if (!end) {
			cal.step = CAL_RANGE;
			cal.step_start_ms = now;
			cal.save = false;
			cal.cancel = false;
			event = GAMEPAD_EVENT_CALIBRATION_RANGE;
			publish = true;
		}
		break;

	case CAL_RANGE:
		if (cal.cancel || (now - cal.step_start_ms) >= CAL_RANGE_TIMEOUT_MS) {
			end = true;
		} else if (cal.save) {
			result = cal;
			save = true;
		}
		break;

	case CAL_IDLE:
	default:
		break;
	}

	k_spin_unlock(&cal_lock, key);

	if (publish) {
		gamepad_event_publish(event);
	}

	if (save) {
		left_stick_cal_end(left_stick_cal_save(&result));
		return;
	}

	if (end) {
		LOG_WRN("Left stick calibration cancelled");
		left_stick_cal_end(false);
		return;
	}

	k_work_reschedule(&cal_work, K_MSEC(CAL_CHECK_PERIOD_MS));
}

void left_stick_calibration_start(void)
{
	k_spinlock_key_t key;
	int ret;

	ret = left_stick_read_defaults();
	if (ret < 0) {
		LOG_ERR("The left stick is not ready (%d)", ret);
		gamepad_event_publish(GAMEPAD_EVENT_CALIBRATION_FAILED);
		return;
	}

	key = k_spin_lock(&cal_lock);

	if (cal.step != CAL_IDLE) {
		k_spin_unlock(&cal_lock, key);
		return;
	}

	cal.step = CAL_WAIT_RELEASE;
	cal.step_start_ms = k_uptime_get();
	cal.released_since_ms = -1;

	k_spin_unlock(&cal_lock, key);

	LOG_INF("Left stick calibration started");
	analog_axis_set_raw_data_cb(stick, left_stick_raw_cb);
	gamepad_event_publish(GAMEPAD_EVENT_CALIBRATION_CENTRE);
	k_work_reschedule(&cal_work, K_NO_WAIT);
}

#else

void left_stick_calibration_start(void)
{
	LOG_WRN("There is no left stick");
}

#endif /* DT_NODE_HAS_STATUS_OKAY(LEFT_STICK_NODE) */

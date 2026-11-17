/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * All the button combinations of the gamepad are here, so that the buttons of every combination can
 * be compared in one place. No combination may be part of another one that can be active at the
 * same time. They only start after a long hold, and the host still sees the buttons.
 *
 * Select + Start is for the system:
 *   + LB + RB      restarts into the boot ROM to write new firmware.
 *   + Home         (held at power-up) erases the stored settings.
 * L3, the stick button, is for the sticks:
 *   + Home         measures the rest position of the right stick again.
 *   + Select       (held at power-up) calibrates the left stick.
 *
 * Home + LB and Home + RB are kept free for a possible trigger and bumper swap.
 */

#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/sys/util.h>

#include "modules/bootloader.h"
#include "util/combo.h"
#include "modules/factory_reset.h"
#include "modules/gamepad_combos.h"
#include "modules/sticks/left_stick.h"
#include "modules/sticks/right_stick.h"

static const struct combo_def gamepad_combos[] = {
	COMBO_DEF("bootloader", COMBO_PHASE_ANY, 3000, bootloader_enter, INPUT_BTN_SELECT,
		  INPUT_BTN_START, INPUT_BTN_TL, INPUT_BTN_TR),
	COMBO_DEF("factory-reset", COMBO_PHASE_POWER_UP, 5000, factory_reset_perform,
		  INPUT_BTN_SELECT, INPUT_BTN_START, INPUT_BTN_MODE),
	COMBO_DEF("recalibrate-right-stick", COMBO_PHASE_ANY, 1500, right_stick_recalibrate,
		  INPUT_BTN_THUMBL, INPUT_BTN_MODE),
	COMBO_DEF("calibrate-left-stick", COMBO_PHASE_POWER_UP, 1000, left_stick_calibration_start,
		  INPUT_BTN_THUMBL, INPUT_BTN_SELECT),
};

int gamepad_combos_init(void)
{
	int ret;

	ret = combo_init(gamepad_combos, ARRAY_SIZE(gamepad_combos));
	if (ret < 0) {
		return ret;
	}

	/* Buttons that are held now were held at power-up, as no event tells about them. */
	combo_seed_from_gpio_keys();

	return 0;
}

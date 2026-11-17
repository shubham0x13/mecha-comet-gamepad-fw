/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_COMBO_H_
#define APP_COMBO_H_

#include <stddef.h>
#include <stdint.h>

/** The most buttons that one combination can have. */
#define COMBO_MAX_BUTTONS 6

/** When a combination is looked for. */
enum combo_phase {
	/** At any time. */
	COMBO_PHASE_ANY,
	/**
	 * Only when all the buttons are pressed within the first moments after power-up, as when
	 * they are held while the USB cable is plugged in.
	 */
	COMBO_PHASE_POWER_UP,
};

/** Definition of a button combination. Use COMBO_DEF() to fill it. */
struct combo_def {
	/** Name for the log. */
	const char *name;
	/** Input key codes (INPUT_BTN_*) of the buttons that make the combination. */
	uint16_t codes[COMBO_MAX_BUTTONS];
	/** Number of codes. */
	uint8_t num_codes;
	/** How long all the buttons must be held, in milliseconds. */
	uint32_t hold_ms;
	/** When the combination is looked for. */
	enum combo_phase phase;
	/**
	 * Called once, after the buttons were held for hold_ms. It runs in the work queue of the
	 * combinations, not the system work queue, so it may block, for example to restart.
	 */
	void (*handler)(void);
};

/**
 * Define a combination.
 *
 * @param _name Name for the log.
 * @param _phase Value of enum combo_phase.
 * @param _hold_ms How long the buttons must be held, in milliseconds.
 * @param _handler Function called when the combination is done.
 * @param ... The input key codes of the buttons.
 */
#define COMBO_DEF(_name, _phase, _hold_ms, _handler, ...)                                          \
	{                                                                                          \
		.name = _name,                                                                     \
		.codes = {__VA_ARGS__},                                                            \
		.num_codes = sizeof((const uint16_t[]){__VA_ARGS__}) / sizeof(uint16_t),           \
		.hold_ms = _hold_ms,                                                               \
		.phase = _phase,                                                                   \
		.handler = _handler,                                                               \
	}

/**
 * Start looking for button combinations.
 *
 * The buttons are followed through the input events, which are not changed or delayed: the host
 * still sees every button press. No combination may be part of another one that can be active at
 * the same time, because the shorter one would always win.
 *
 * @param defs Definitions of the combinations. They must stay valid after this call returns.
 * @param count Number of definitions.
 *
 * @retval 0 on success.
 * @retval -EINVAL if a combination is empty, has no handler, has more than COMBO_MAX_BUTTONS
 * buttons or is part of another one.
 * @retval -ENOSPC if there are more than CONFIG_COMET_GAMEPAD_COMBO_MAX combinations.
 */
int combo_init(const struct combo_def *defs, size_t count);

/**
 * Tell that a button is pressed or released without an input event.
 *
 * This is how the buttons that were already held at power-up are given, because the input
 * drivers only report changes.
 *
 * @param code Input key code of the button.
 * @param pressed True when the button is pressed.
 */
void combo_set_key(uint16_t code, bool pressed);

/**
 * Get the number of buttons that are held now.
 *
 * It includes the buttons that were held at power-up, once combo_seed_from_gpio_keys() ran.
 *
 * @return Number of buttons held.
 */
size_t combo_keys_held(void);

/**
 * Give the combinations the buttons that are held now, by reading the pins of all the
 * "gpio-keys" buttons.
 */
void combo_seed_from_gpio_keys(void);

#endif /* APP_COMBO_H_ */

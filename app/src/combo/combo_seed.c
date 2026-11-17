/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * The gpio-keys driver reads the state of its pins when it starts and only reports changes after
 * that. A button that is held at power-up never produces a press event, so the combinations that
 * are made at power-up need the pins to be read here.
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "combo/combo.h"

LOG_MODULE_DECLARE(combo, CONFIG_COMET_GAMEPAD_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(gpio_keys)

struct seed_button {
	struct gpio_dt_spec spec;
	uint16_t code;
};

#define SEED_BUTTON(node_id)                                                                       \
	{.spec = GPIO_DT_SPEC_GET(node_id, gpios), .code = DT_PROP(node_id, zephyr_code)},
#define SEED_NODE(node_id) DT_FOREACH_CHILD_STATUS_OKAY(node_id, SEED_BUTTON)

static const struct seed_button seed_buttons[] = {DT_FOREACH_STATUS_OKAY(gpio_keys, SEED_NODE)};

void combo_seed_from_gpio_keys(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(seed_buttons); i++) {
		const struct seed_button *button = &seed_buttons[i];
		int ret;

		if (!gpio_is_ready_dt(&button->spec)) {
			LOG_WRN("Button %u is not ready", button->code);
			continue;
		}

		ret = gpio_pin_get_dt(&button->spec);
		if (ret > 0) {
			combo_set_key(button->code, true);
		}
	}
}

#else

void combo_seed_from_gpio_keys(void)
{
}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(gpio_keys) */

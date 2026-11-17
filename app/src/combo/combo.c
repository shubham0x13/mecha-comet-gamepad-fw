/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdbool.h>

#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "combo/combo.h"

LOG_MODULE_REGISTER(combo, CONFIG_COMET_GAMEPAD_LOG_LEVEL);

/* The input key and button codes of Zephyr are below this value. Others are not followed. */
#define COMBO_CODE_COUNT 0x300

struct combo_state {
	const struct combo_def *def;
	struct k_work_delayable work;
	/* True from the moment all the buttons are pressed until one is released. */
	bool armed;
};

static struct combo_state states[CONFIG_COMET_GAMEPAD_COMBO_MAX];
/* Guards the armed flags, as keys come from the input thread and, at power-up, the main thread. */
static struct k_spinlock lock;
static size_t state_count;
static ATOMIC_DEFINE(pressed, COMBO_CODE_COUNT);
static atomic_t ready;

K_THREAD_STACK_DEFINE(combo_stack, CONFIG_COMET_GAMEPAD_COMBO_STACK_SIZE);
static struct k_work_q combo_work_q;

static bool combo_is_held(const struct combo_def *def)
{
	for (size_t i = 0; i < def->num_codes; i++) {
		if (!atomic_test_bit(pressed, def->codes[i])) {
			return false;
		}
	}

	return true;
}

static bool combo_has_code(const struct combo_def *def, uint16_t code)
{
	for (size_t i = 0; i < def->num_codes; i++) {
		if (def->codes[i] == code) {
			return true;
		}
	}

	return false;
}

static bool combo_is_part_of(const struct combo_def *part, const struct combo_def *whole)
{
	for (size_t i = 0; i < part->num_codes; i++) {
		if (!combo_has_code(whole, part->codes[i])) {
			return false;
		}
	}

	return true;
}

static bool combo_phases_overlap(const struct combo_def *a, const struct combo_def *b)
{
	return (a->phase == COMBO_PHASE_ANY) || (b->phase == COMBO_PHASE_ANY) ||
	       (a->phase == b->phase);
}

static bool combo_power_up_window_is_open(void)
{
	return k_uptime_get() < CONFIG_COMET_GAMEPAD_COMBO_POWER_UP_WINDOW_MS;
}

static void combo_update(struct combo_state *state)
{
	const struct combo_def *def = state->def;
	bool held = combo_is_held(def);

	if (held && !state->armed) {
		if (def->phase == COMBO_PHASE_POWER_UP && !combo_power_up_window_is_open()) {
			return;
		}

		state->armed = true;
		k_work_schedule_for_queue(&combo_work_q, &state->work, K_MSEC(def->hold_ms));
	} else if (!held && state->armed) {
		state->armed = false;
		k_work_cancel_delayable(&state->work);
	}
}

static void combo_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct combo_state *state = CONTAINER_OF(dwork, struct combo_state, work);

	/* A button may have been released just before this ran. */
	if (!combo_is_held(state->def)) {
		return;
	}

	LOG_INF("Combination %s", state->def->name);
	state->def->handler();
}

void combo_set_key(uint16_t code, bool pressed_now)
{
	k_spinlock_key_t key;

	if (!atomic_get(&ready) || code >= COMBO_CODE_COUNT) {
		return;
	}

	key = k_spin_lock(&lock);

	if (pressed_now) {
		atomic_set_bit(pressed, code);
	} else {
		atomic_clear_bit(pressed, code);
	}

	for (size_t i = 0; i < state_count; i++) {
		combo_update(&states[i]);
	}

	k_spin_unlock(&lock, key);
}

size_t combo_keys_held(void)
{
	size_t count = 0;

	for (size_t i = 0; i < ATOMIC_BITMAP_SIZE(COMBO_CODE_COUNT); i++) {
		/* atomic_val_t is a long, which has 64 bits on 64-bit hosts. */
		count += (size_t)__builtin_popcountl((unsigned long)atomic_get(&pressed[i]));
	}

	return count;
}

static void combo_input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	if (evt->type == INPUT_EV_KEY) {
		combo_set_key(evt->code, evt->value != 0);
	}
}
INPUT_CALLBACK_DEFINE(NULL, combo_input_cb, NULL);

int combo_init(const struct combo_def *defs, size_t count)
{
	if (count > ARRAY_SIZE(states)) {
		return -ENOSPC;
	}

	for (size_t i = 0; i < count; i++) {
		if (defs[i].num_codes == 0 || defs[i].num_codes > COMBO_MAX_BUTTONS ||
		    defs[i].handler == NULL) {
			LOG_ERR("Combination %s is not complete", defs[i].name);
			return -EINVAL;
		}

		for (size_t j = 0; j < count; j++) {
			if (i != j && combo_phases_overlap(&defs[i], &defs[j]) &&
			    combo_is_part_of(&defs[i], &defs[j])) {
				LOG_ERR("Combination %s is part of %s", defs[i].name, defs[j].name);
				return -EINVAL;
			}
		}
	}

	if (!atomic_get(&ready)) {
		const struct k_work_queue_config cfg = {.name = "combo"};

		k_work_queue_start(&combo_work_q, combo_stack, K_THREAD_STACK_SIZEOF(combo_stack),
				   CONFIG_COMET_GAMEPAD_COMBO_THREAD_PRIORITY, &cfg);
	}

	for (size_t i = 0; i < count; i++) {
		states[i].def = &defs[i];
		states[i].armed = false;
		k_work_init_delayable(&states[i].work, combo_work_handler);
	}

	state_count = count;
	atomic_set(&ready, 1);

	return 0;
}

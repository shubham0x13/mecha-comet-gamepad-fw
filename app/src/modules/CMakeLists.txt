# Copyright (c) 2026 Mecha Systems
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(sticks)
add_subdirectory(usb)

target_sources(app PRIVATE
	bootloader.c
	factory_reset.c
	gamepad_combos.c
	gamepad_input.c
	storage.c
)

target_sources_ifdef(CONFIG_COMET_GAMEPAD_STATUS_LED app PRIVATE status_led.c)

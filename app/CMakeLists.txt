# Copyright (c) 2026 Mecha Systems
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.28.0)

# The build variants in snippets/ are used with `west build -S <name>`.
list(APPEND SNIPPET_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(comet_gamepad)

# Headers are included with their folder, for example "modules/usb/gamepad_hid.h".
target_include_directories(app PRIVATE src)

target_sources(app PRIVATE src/main.c)

# events: how the parts tell each other what happened.
# modules: what the gamepad does, one file per part.
# util: code that knows nothing about the gamepad, used by the modules.
add_subdirectory(src/events)
add_subdirectory(src/modules)
add_subdirectory(src/util)

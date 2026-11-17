/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_RIGHT_STICK_H_
#define APP_RIGHT_STICK_H_

/**
 * Measure the rest position of the right stick (the Pivot Pip) again.
 *
 * The stick must not be touched while this runs, which takes about 100 ms. It does nothing when
 * the Pivot Pip is not in the devicetree or is disabled.
 */
void right_stick_recalibrate(void);

#endif /* APP_RIGHT_STICK_H_ */

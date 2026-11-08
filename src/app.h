/*
 * Copyright (c) 2026 Dante Sangiorgio
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_H_
#define APP_H_

/**
 * @brief Initialise application resources (GPIO, sensors, timers, ...).
 *
 * @return 0 on success, negative errno on failure.
 */
int app_init(void);

/**
 * @brief Run the application main loop.
 *
 * Does not return under normal operation.
 */
void app_run(void);

#endif /* APP_H_ */

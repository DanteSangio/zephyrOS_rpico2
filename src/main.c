/*
 * Copyright (c) 2026 Dante Sangiorgio
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app.h"

LOG_MODULE_REGISTER(pico2_app, LOG_LEVEL_INF);

int main(void)
{
	int ret;

	LOG_INF("booting on %s", CONFIG_BOARD_TARGET);

	ret = app_init();
	if (ret < 0) {
		LOG_ERR("app_init() failed (%d), halting", ret);
		return ret;
	}

	LOG_INF("app_init() done, entering main loop");
	app_run();

	return 0;
}

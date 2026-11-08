/*
 * Copyright (c) 2026 Dante Sangiorgio
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "app.h"

/* Defined (and registered) in main.c, declared here: one log module per app. */
LOG_MODULE_DECLARE(pico2_app);

#define LED0_NODE       DT_ALIAS(led0)
#define BLINK_PERIOD_MS 1000

#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "This application needs an enabled led0 devicetree alias"
#endif

/* The Pico 2 user LED is GPIO25, exposed by the board as led0. */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int app_init(void)
{
	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("GPIO controller for led0 is not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE) < 0) {
		LOG_ERR("failed to configure led0 as an output");
		return -EIO;
	}

	return 0;
}

void app_run(void)
{
	uint32_t heartbeat = 0U;

	while (true) {
		bool on = (heartbeat % 2U) == 0U;

		if (gpio_pin_set_dt(&led, on) < 0) {
			LOG_ERR("failed to drive led0");
			return;
		}

		LOG_INF("heartbeat %u: LED %s", heartbeat, on ? "ON" : "OFF");
		heartbeat++;
		k_msleep(BLINK_PERIOD_MS);
	}
}

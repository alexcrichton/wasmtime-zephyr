/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <app_version.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

#ifndef CONFIG_USERSPACE
#error This requires CONFIG_USERSPACE.
#endif

/* #include <custom_lib/custom_lib.h> */

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

#define USER_STACKSIZE	2048
struct k_thread user_thread;
K_THREAD_STACK_DEFINE(user_stack, USER_STACKSIZE);

static void user_function(void *p1, void *p2, void *p3) {
  printf("Hello World\n");
}

int main(void)
{
	printk("Zephyr Example Application %s\n", APP_VERSION_STRING);
        /* printk("%d\n", custom_lib_get_value(1)); */

	k_thread_create(&user_thread, user_stack, USER_STACKSIZE,
			user_function, NULL, NULL, NULL,
			-1, K_USER, K_MSEC(0));
	return 0;
}


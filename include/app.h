#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdlib.h>
#include <zephyr/drivers/i2c.h>

const static uint8_t _I2C_MSG_READ = I2C_MSG_READ;
const static uint8_t _I2C_MSG_STOP = I2C_MSG_STOP;

struct device;

void app_printk(const uint8_t *ptr, size_t len);

__attribute__((noreturn))
void app_abort(void);

void* app_alloc(size_t size, size_t align);
void app_dealloc(void *ptr);

#endif // APP_H

#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdlib.h>

void app_printk(const uint8_t *ptr, size_t len);

__attribute__((noreturn))
void app_abort(void);

void* app_alloc(size_t size, size_t align);
void app_dealloc(void *ptr);

#endif // APP_H

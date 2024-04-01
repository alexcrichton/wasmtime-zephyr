#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdlib.h>

void app_printk_rust(const uint8_t *ptr, size_t len);

__attribute__((noreturn))
void app_abort(void);

#endif // APP_H

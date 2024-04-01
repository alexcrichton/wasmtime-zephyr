#include <zephyr/kernel.h>
#include <app_version.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/logging/log_ctrl.h>
#include <stdio.h>
#include <stdlib.h>

#include "app.h"
#include "wasmtime.h"

struct k_mem_domain wasmtime_domain;

static void user_thread(void *p1, void *p2, void *p3) {
  rust_foo();
}

#define MY_STACK_SIZE (512 << 10)
#define MY_PRIORITY -5
K_THREAD_DEFINE(my_tid, MY_STACK_SIZE,
                user_thread, NULL, NULL, NULL,
                MY_PRIORITY, K_USER, -1);

int main(void) {
  /* LOG_INIT(); */
  printk("Zephyr Example Application %s\n", APP_VERSION_STRING);

  struct k_mem_partition *parts[] = {
    &z_libc_partition,
    &z_malloc_partition,
  };
  k_mem_domain_init(&wasmtime_domain, 2, parts);
  k_mem_domain_add_thread(&wasmtime_domain, my_tid);
  k_thread_start(my_tid);
  return 0;
}

void app_printk(const uint8_t *ptr, size_t len) {
  printk("%.*s", (int) len, (const char*) ptr);
}

void app_abort() {
  abort();
}

void* app_alloc(size_t size, size_t align) {
  return aligned_alloc(align, size);
}

void app_dealloc(void *ptr) {
  free(ptr);
}

size_t app_page_size() {
  return CONFIG_MMU_PAGE_SIZE;
}

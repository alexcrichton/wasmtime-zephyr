#include <zephyr/kernel.h>
#include <app_version.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/logging/log_ctrl.h>
#include <stdio.h>
#include <stdlib.h>

#include "app.h"
#include "wasmtime.h"
#include "wasmtime_syscalls.h"

static void user_thread(void *p1, void *p2, void *p3) {
  int a = 1;
  int b = 2;
  int result = rust_add(a, b);

  printk("according to wasm %d + %d = %d\n", a, b, result);

  // NB: refer to `rust_run` in a way it can't be optimized away.
  // TODO: how to create a `const struct device*` to run i2c stuff on?
  if (result == 4)
    rust_run(NULL);
}

#define MY_STACK_SIZE (512 << 10)
#define MY_PRIORITY -5
K_THREAD_DEFINE(my_tid, MY_STACK_SIZE,
                user_thread, NULL, NULL, NULL,
                MY_PRIORITY, K_USER, -1);

int main(void) {
  struct k_mem_partition *parts[] = {
    &z_libc_partition,
    &z_malloc_partition,
  };
  k_mem_domain_init(&wasmtime_domain, 2, parts);
  k_mem_domain_add_thread(&wasmtime_domain, my_tid);
  k_thread_start(my_tid);
  return 0;
}

// Used in rust panic messages to print things.
void app_printk(const uint8_t *ptr, size_t len) {
  // TODO: is there a more appropriate stream to use other than `printk`?
  printk("%.*s", (int) len, (const char*) ptr);
}

// Used in rust to to abort on a panic.
void app_abort() {
  abort();
}

// Memory allocation for Rust
void* app_alloc(size_t size, size_t align) {
  return aligned_alloc(align, size);
}

// Memory deallocation for Rust
void app_dealloc(void *ptr) {
  free(ptr);
}

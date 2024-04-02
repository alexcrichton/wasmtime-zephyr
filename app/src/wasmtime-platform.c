#include <zephyr/kernel.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <assert.h>
#include <wasmtime-platform.h>

#include "wasmtime_syscalls.h"

int wasmtime_mmap_new(uintptr_t size, uint32_t prot_flags, uint8_t **ret) {
  return mmap_new(size, prot_flags, (void**) ret);
}

int wasmtime_mmap_remap(uint8_t *addr, uintptr_t size, uint32_t prot_flags) {
  return mmap_remap(addr, size, prot_flags);
}

int wasmtime_munmap(uint8_t *ptr, uintptr_t size) {
  return mmap_munmap(ptr, size);
}

int wasmtime_mprotect(uint8_t *ptr, uintptr_t size, uint32_t prot_flags) {
  return mmap_mprotect(ptr, size, prot_flags);
}

uintptr_t wasmtime_page_size(void) {
  return CONFIG_MMU_PAGE_SIZE;
}

int32_t wasmtime_setjmp(const uint8_t **jmp_buf_out,
                        void (*callback)(uint8_t *, uint8_t *),
                        uint8_t *payload, uint8_t *callee) {
  // TODO: fill this in
  callback(payload, callee);
  return 1;
}

void wasmtime_longjmp(const uint8_t *jmp_buf_ptr) {
  // TODO: fill this in
  abort();
}

int wasmtime_init_traps(wasmtime_trap_handler_t handler) {
  // TODO: fill this in
  return 0;
}

int wasmtime_memory_image_new(const uint8_t *ptr, uintptr_t len,
                              struct wasmtime_memory_image **ret) {
  *ret = NULL;
  return 0;
}

int wasmtime_memory_image_map_at(struct wasmtime_memory_image *image,
                                 uint8_t *addr, uintptr_t len) {
  abort();
}

void wasmtime_memory_image_free(struct wasmtime_memory_image *image) {
  abort();
}

// NB: only one thread right now so this is just a `static`, not actually thread
// local.
K_APP_BMEM(z_libc_partition) static uint8_t *wasmtime_tls = NULL;

uint8_t* wasmtime_tls_get(void) {
  return wasmtime_tls;
}

void wasmtime_tls_set(uint8_t *ptr) {
  wasmtime_tls = ptr;
}

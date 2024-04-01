#ifndef WASMTIME_SYSCALLS_H
#define WASMTIME_SYSCALLS_H

#include <zephyr/toolchain.h>
#include <stdint.h>
#include <stddef.h>

__syscall int mmap_new(size_t size, uint32_t flags, void **ret);
__syscall int mmap_remap(void *addr, size_t size, uint32_t flags);
__syscall int mmap_munmap(void *ptr, size_t size);
__syscall int mmap_mprotect(void *ptr, size_t size, uint32_t prot);

#include <syscalls/wasmtime_syscalls.h>

#endif /* WASMTIME_SYSCALLS_H */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/logging/log.h>
#include "wasmtime_syscalls.h"
#include "wasmtime-platform.h"

LOG_MODULE_REGISTER(wasmtime_syscalls);

int z_impl_mmap_new(size_t size, uint32_t flags, void **ret) {
  uint32_t zephyr_flags = 0;
  if ((flags & WASMTIME_PROT_READ) != 0)
    zephyr_flags |= K_MEM_PERM_USER;
  if ((flags & WASMTIME_PROT_WRITE) != 0)
    zephyr_flags |= K_MEM_PERM_RW;
  if ((flags & WASMTIME_PROT_EXEC) != 0)
    zephyr_flags |= K_MEM_PERM_EXEC;

  void *ptr = k_mem_map(size, zephyr_flags);
  if (ptr == NULL) {
    LOG_ERR("failed to allocate memory");
    *ret = NULL;
    return -ENOMEM;
  }

  *ret = ptr;
  return 0;
}

static int z_vrfy_mmap_new(size_t size, uint32_t flags, void **ret) {
  void *my_ret = NULL;
  if (size % CONFIG_MMU_PAGE_SIZE != 0) {
    LOG_ERR("size is not page-aligned");
    return -EINVAL;
  }
  int rc = z_impl_mmap_new(size, flags, &my_ret);
  K_OOPS(k_usermode_to_copy(ret, &my_ret, sizeof(my_ret)));
  return rc;
}

int z_impl_mmap_remap(void *addr, size_t size, uint32_t flags) {
  LOG_ERR("unimplemented remap");
  return -EINVAL;
}

static int z_vrfy_mmap_remap(void *addr, size_t size, uint32_t flags) {
  return z_impl_mmap_remap(addr, size, flags);
}

int z_impl_mmap_munmap(void *addr, size_t size) {
  k_mem_unmap(addr, size);
  return 0;
}

static int z_vrfy_mmap_munmap(void *addr, size_t size) {
  return z_impl_mmap_munmap(addr, size);
}

int z_impl_mmap_mprotect(void *addr, size_t size, uint32_t flags) {
  LOG_ERR("unimplemented mmap_mprotect");
  return -EINVAL;
}

static int z_vrfy_mmap_mprotect(void *addr, size_t size, uint32_t flags) {
  return z_impl_mmap_mprotect(addr, size, flags);
}

#include <syscalls/mmap_mprotect_mrsh.c>
#include <syscalls/mmap_munmap_mrsh.c>
#include <syscalls/mmap_new_mrsh.c>
#include <syscalls/mmap_remap_mrsh.c>

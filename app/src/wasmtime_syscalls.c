#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/logging/log.h>
#include "wasmtime_syscalls.h"
#include "wasmtime-platform.h"

#define MAX_PARTITIONS 8

LOG_MODULE_REGISTER(wasmtime_syscalls);

struct k_mem_domain wasmtime_domain;

// This heap is where "mmap'd memory" is allocated from.
//
// If this could be in flash memory then it should. I don't know how to do that
// in zephyr right now so it's instead here in the data section.
K_HEAP_DEFINE(wasmtime_heap, CONFIG_WASMTIME_HEAP_SIZE);

struct wasmtime_mmap {
  uint8_t *base;
  size_t len;
  struct k_mem_partition partitions[MAX_PARTITIONS];
  size_t num_partitions;
};

static struct {
  size_t num_mmaps;
  struct wasmtime_mmap mmaps[CONFIG_WASMTIME_NUM_MMAPS];
} wasmtime_mmaps = {0};

// Find the mmap in `wasmtime_mmaps` which contains `base`.
static int find_mmap_containing(uint8_t *base) {
  for (int i = 0; i < wasmtime_mmaps.num_mmaps; i++) {
    struct wasmtime_mmap *map = &wasmtime_mmaps.mmaps[i];
    if (map->base <= base && base < map->base + map->len)
      return i;
  }
  LOG_ERR("failed to find mmap");
  return -EINVAL;
}

// Converts wasmtime `flags` to zephyr `k_mem_partition_attr_t`.
static k_mem_partition_attr_t flags_to_attrs(uint32_t flags) {
  if (flags == (WASMTIME_PROT_READ | WASMTIME_PROT_WRITE)) {
    return K_MEM_PARTITION_P_RW_U_RW;
  }
  if (flags == (WASMTIME_PROT_READ | WASMTIME_PROT_EXEC)) {
    return K_MEM_PARTITION_P_RX_U_RX;
  }
  if (flags == WASMTIME_PROT_READ) {
    return K_MEM_PARTITION_P_RO_U_RO;
  }
  if (flags == 0) {
    return K_MEM_PARTITION_P_RO_U_NA;
  }
  LOG_ERR("invalid permission flags for new mmap: %x", flags);
  abort();
}

// Adds a new partition to both `map` and `wasmtime_domain`.
static int add_partition(struct wasmtime_mmap *map,
                         void *base,
                         size_t size,
                         k_mem_partition_attr_t attr) {
  if (size == 0)
    return 0;
  if (map->num_partitions == MAX_PARTITIONS) {
    LOG_ERR("cannot add more partitions");
    return -ENOMEM;
  }
  struct k_mem_partition *partition = &map->partitions[map->num_partitions];
  partition->start = (uintptr_t) base;
  partition->size = size;
  partition->attr = attr;

  map->num_partitions += 1;
  return k_mem_domain_add_partition(&wasmtime_domain, partition);
}

int z_impl_mmap_new(size_t size, uint32_t flags, void **ret) {
  if (wasmtime_mmaps.num_mmaps == CONFIG_WASMTIME_NUM_MMAPS) {
    LOG_ERR("no more space for mmaps");
    return -ENOMEM;
  }
  struct wasmtime_mmap *map = &wasmtime_mmaps.mmaps[wasmtime_mmaps.num_mmaps];

  map->base = k_heap_aligned_alloc(&wasmtime_heap, CONFIG_MMU_PAGE_SIZE, size, K_NO_WAIT);

  if (map->base == NULL) {
    LOG_ERR("no more memory to allocate");
    return -ENOMEM;
  }
  map->len = size;
  map->num_partitions = 0;
  k_mem_partition_attr_t attr = flags_to_attrs(flags);
  int rc = add_partition(map, map->base, size, attr);
  if (rc != 0) {
    k_heap_free(&wasmtime_heap, map->base);
    return rc;
  }

  wasmtime_mmaps.num_mmaps++;
  *ret = map->base;
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
  int mapi = find_mmap_containing(addr);
  if (mapi < 0)
    return mapi;
  struct wasmtime_mmap *map = &wasmtime_mmaps.mmaps[mapi];
  if (map->base != addr || map->len != size) {
    LOG_ERR("failed to find mmap");
    return -EINVAL;
  }
  for (int i = 0; i < map->num_partitions; i++) {
    k_mem_domain_remove_partition(&wasmtime_domain, &map->partitions[i]);
  }
  k_heap_free(&wasmtime_heap, map->base);
  if (wasmtime_mmaps.num_mmaps > 1) {
    wasmtime_mmaps.mmaps[mapi] = wasmtime_mmaps.mmaps[wasmtime_mmaps.num_mmaps - 1];
  }
  wasmtime_mmaps.num_mmaps -= 1;
  return 0;
}

static int z_vrfy_mmap_munmap(void *addr, size_t size) {
  return z_impl_mmap_munmap(addr, size);
}

static size_t round_up(size_t size) {
  return (size + (CONFIG_MMU_PAGE_SIZE - 1)) & ~(CONFIG_MMU_PAGE_SIZE - 1);
}

int z_impl_mmap_mprotect(void *addr, size_t not_rounded_size, uint32_t flags) {
  size_t size = round_up(not_rounded_size);
  int mapi = find_mmap_containing(addr);
  if (mapi < 0)
    return mapi;

  struct wasmtime_mmap *map = &wasmtime_mmaps.mmaps[mapi];
  if (size > map->len) {
    LOG_ERR("size too large");
    return -EINVAL;
  }
  int rc = 0;

  int num_partitions = map->num_partitions;
  map->num_partitions = 0;
  for (int i = 0; i < num_partitions; i++) {
    rc = k_mem_domain_remove_partition(&wasmtime_domain, &map->partitions[i]);
    if (rc != 0) {
      LOG_ERR("failed to remove partition");
      return rc;
    }
  }

  // handle changing the permissions of the entire mmap
  if (addr == map->base && size == map->len) {
    return add_partition(map, addr, size, flags_to_attrs(flags));
  }

  // handle splitting a single mmap
  if (num_partitions == 1) {
    k_mem_partition_attr_t prev_attrs = map->partitions[0].attr;
    uint8_t *base = map->base;
    size_t base_len = ((uintptr_t) addr) - ((uintptr_t) base);
    uint8_t *mid = base + base_len;
    size_t mid_len = size;
    uint8_t *end = mid + mid_len;
    size_t end_len = map->len - mid_len - base_len;

    rc = add_partition(map, base, base_len, prev_attrs);
    if (rc == 0)
      rc = add_partition(map, mid, mid_len, flags_to_attrs(flags));
    if (rc == 0)
      rc = add_partition(map, end, end_len, prev_attrs);
    return rc;
  }

  LOG_ERR("unimplemented mmap_mprotect configuration");
  return -EINVAL;
}

static int z_vrfy_mmap_mprotect(void *addr, size_t size, uint32_t flags) {
  return z_impl_mmap_mprotect(addr, size, flags);
}

#include <syscalls/mmap_mprotect_mrsh.c>
#include <syscalls/mmap_munmap_mrsh.c>
#include <syscalls/mmap_new_mrsh.c>
#include <syscalls/mmap_remap_mrsh.c>

#ifndef _ION_ALLOC_H_
#define _ION_ALLOC_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <pthread.h>
#include <asm-generic/ioctl.h>
#include "ion_alloc_5.4.h"

#if ((defined CONF_KERNEL_VERSION_4_9) || \
     (defined CONF_KERNEL_VERSION_4_4) || \
     (defined CONF_KERNEL_VERSION_3_10) || \
     (defined CONF_KERNEL_VERSION_3_4))
#define CONF_KERNEL_VERSION_OLD 1
#endif

#if !defined(CONF_KERNEL_VERSION_3_4)
    typedef int aw_ion_user_handle_t;
#else
    typedef void* aw_ion_user_handle_t;
#endif


typedef struct aw_ion_allocation_info {
    size_t aw_len;
    size_t aw_align;
    unsigned int aw_heap_id_mask;
    unsigned int flags;
    aw_ion_user_handle_t handle;
} aw_ion_allocation_info_t;

typedef struct ion_handle_data {
    aw_ion_user_handle_t handle;
} ion_handle_data_t ;

typedef struct aw_ion_fd_data {
    aw_ion_user_handle_t handle;
    int aw_fd;
} ion_fd_data_t;

typedef struct aw_ion_custom_info {
    unsigned int aw_cmd;
    unsigned long aw_arg;
} ion_custom_data_t;

typedef struct SUNXI_PHYS_DATA {
    aw_ion_user_handle_t handle;
    unsigned int  phys_addr;
    unsigned int  size;
} sunxi_phys_data;


typedef struct {
#if CONF_KERNEL_VERSION_OLD
    long    start;
    long    end;
#else
    long long    start;
    long long    end;
#endif
} sunxi_cache_range;

#define SZ_64M 0x04000000
#define SZ_4M 0x00400000
#define SZ_1M 0x00100000
#define SZ_64K 0x00010000
#define SZ_4k 0x00001000
#define SZ_1k 0x00000400

enum ion_heap_type {
    AW_ION_SYSTEM_HEAP_TYPE,
    AW_ION_SYSTEM_CONTIG_HEAP_TYPE,
    AW_ION_CARVEOUT_HEAP_TYPE,
    AW_ION_TYPE_HEAP_CHUNK,
    AW_ION_TYPE_HEAP_DMA,
    AW_ION_TYPE_HEAP_CUSTOM,

    AW_ION_TYPE_HEAP_SECURE,

    AW_ION_NUM_HEAPS = 16,
};

#define AW_MEM_ION_IOC_MAGIC 'I'
#define AW_MEM_ION_IOC_ALLOC _IOWR(AW_MEM_ION_IOC_MAGIC, \
		0, struct aw_ion_allocation_info)
#define AW_MEM_ION_IOC_FREE _IOWR(AW_MEM_ION_IOC_MAGIC, 1, \
		struct ion_handle_data)
#define AW_MEM_ION_IOC_MAP _IOWR(AW_MEM_ION_IOC_MAGIC, 2, \
		struct aw_ion_fd_data)
#define AW_MEMION_IOC_SHARE _IOWR(AW_MEM_ION_IOC_MAGIC, 4, \
		struct ion_fd_data)
#define AW_MEMION_IOC_IMPORT _IOWR(AW_MEM_ION_IOC_MAGIC, 5, \
		struct ion_fd_data)
#define AW_MEMION_IOC_SYNC _IOWR(AW_MEM_ION_IOC_MAGIC, 7, \
		struct ion_fd_data)
#define AW_MEM_ION_IOC_CUSTOM _IOWR(AW_MEM_ION_IOC_MAGIC, 6, \
		struct aw_ion_custom_info)

#define AW_MEM_ENGINE_REQ 0x206
#define AW_MEM_ENGINE_REL 0x207
#define AW_MEM_GET_IOMMU_ADDR	0x502
#define AW_MEM_FREE_IOMMU_ADDR	0x503
/* for flush cache range since kernel 5.4 */
#define AW_MEM_FLUSH_CACHE_RANGE	0x506

#define AW_ION_CACHED_FLAG 1
#define AW_ION_CACHED_NEEDS_SYNC_FLAG 2

#define ION_IOC_SUNXI_FLUSH_RANGE           5
#define ION_IOC_SUNXI_FLUSH_ALL             6
#define ION_IOC_SUNXI_PHYS_ADDR             7
#define ION_IOC_SUNXI_DMA_COPY              8

#define ION_IOC_SUNXI_TEE_ADDR              17
#define AW_ION_SYSTEM_HEAP_MASK        (1 << AW_ION_SYSTEM_HEAP_TYPE)
#define AW_ION_SYSTEM_CONTIG_HEAP_MASK    (1 << AW_ION_SYSTEM_CONTIG_HEAP_TYPE)
#define AW_ION_CARVEOUT_HEAP_MASK        (1 << AW_ION_CARVEOUT_HEAP_TYPE)
#define AW_ION_DMA_HEAP_MASK        (1 << AW_ION_TYPE_HEAP_DMA)
#define AW_ION_SECURE_HEAP_MASK      (1 << AW_ION_TYPE_HEAP_SECURE)

#endif

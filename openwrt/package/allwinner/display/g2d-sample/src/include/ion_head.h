/*
 * ion_head.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sunxi ion test head file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef ION_HEAD__H
#define ION_HEAD__H

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
#include <sys/mman.h>
#include <asm-generic/ioctl.h>

typedef char s8;
typedef unsigned char u8;

typedef short s16;
typedef unsigned short u16;

typedef int32_t s32;
typedef uint32_t u32;
typedef uint64_t u64;

#define ION_TEST_ID_BASE                     	 100
#define ION_TEST_FUNC_ALLOC                      (ION_TEST_ID_BASE + 1)
#define ION_TEST_FUNC_FREE                       (ION_TEST_ID_BASE + 2)
#define ION_TEST_FUNC_SHARE                      (ION_TEST_ID_BASE + 3)
#define ION_TEST_FUNC_IMPORT                     (ION_TEST_ID_BASE + 4)
#define ION_TEST_FUNC_SYNC                       (ION_TEST_ID_BASE + 5)
#define ION_TEST_FUNC_FLUSH_RANGE                (ION_TEST_ID_BASE + 6)
#define ION_TEST_FUNC_PHYS_ADDR                  (ION_TEST_ID_BASE + 7)
#define ION_TEST_FUNC_DMA_COPY                   (ION_TEST_ID_BASE + 8)
#define ION_TEST_FUNC_DUMP_CARVEOUT_BITMAP       (ION_TEST_ID_BASE + 9)

int ion_function_alloc(void);
int ion_function_free(void);
int ion_function_share(void);
int ion_function_import(void);
int ion_function_sync(void);
int ion_function_flush_range(void);
int ion_function_phys_addr(void);
int ion_function_dma_copy(void);
int ion_function_dump_carveout_bitmap(void);

#define ION_DEV_NAME		"/dev/ion"

#define SZ_64M			0x04000000
#define SZ_4M			0x00400000
#define SZ_1M			0x00100000
#define SZ_64K			0x00010000
#define SZ_4K			0x00001000
#define ION_ALLOC_SIZE		(SZ_4M + SZ_1M - SZ_64K)
#define ION_ALLOC_ALIGN		(SZ_4K)

typedef int ion_user_handle_t;

struct ion_allocation_data {
	u64 len;
	u32 heap_id_mask;
	u32 flags;
	u32 fd;
	u32 unused;
};

struct ion_handle_data {
	ion_user_handle_t handle;
};

struct ion_fd_data {
	ion_user_handle_t handle;
	int fd;
};

struct ion_custom_data {
	unsigned int cmd;
	unsigned long arg;
};

/* standard cmd from kernel head file */
#define ION_IOC_MAGIC 'I'
#define ION_IOC_ALLOC		_IOWR(ION_IOC_MAGIC, 0, struct ion_allocation_data)
#define ION_IOC_HEAP_QUERY	_IOWR(ION_IOC_MAGIC, 8, struct ion_heap_query)
#define ION_IOC_ABI_VERSION	_IOR(ION_IOC_MAGIC, 9, __u32)

/* extend cmd of sunxi platform */
#define ION_IOC_SUNXI_FLUSH_RANGE       5
#define ION_IOC_SUNXI_FLUSH_ALL         6
#define ION_IOC_SUNXI_PHYS_ADDR         7
#define ION_IOC_SUNXI_DMA_COPY          8
#define ION_IOC_SUNXI_DUMP              9

enum ion_heap_type {
	ION_SYSTEM_HEAP_TYPE,
	ION_TYPE_HEAP_DMA,
};

#define ION_SYSTEM_HEAP_MASK (1 << ION_SYSTEM_HEAP_TYPE) //system堆，iommu使用
#define ION_DMA_HEAP_MASK (1 << ION_TYPE_HEAP_DMA) //cma堆

#define ION_FLAG_CACHED 1		/* mappings of this buffer should be
					   cached, ion will do cache
					   maintenance when the buffer is
					   mapped for dma */
#define ION_FLAG_CACHED_NEEDS_SYNC 2	/* mappings of this buffer will created
					   at mmap time, if this is set
					   caches must be managed manually */

/* used for ION_IOC_SUNXI_FLUSH_RANGE cmd */
typedef struct {
	long 	start;			/* start virtual address */
	long 	end;			/* end virtual address */
}sunxi_cache_range;

/* used for ION_IOC_SUNXI_PHYS_ADDR cmd */
typedef struct {
	ion_user_handle_t handle;
	unsigned int phys_addr;
	unsigned int size;
}sunxi_phys_data;

#define DMA_BUF_BASE 'b'
#define DMA_BUF_IOCTL_SYNC _IOW(DMA_BUF_BASE, 0, struct dma_buf_sync)
struct dma_buf_sync {
	u64 flags;
};
#define DMA_BUF_SYNC_READ (1 << 0)
#define DMA_BUF_SYNC_WRITE (2 << 0)
#define DMA_BUF_SYNC_RW (DMA_BUF_SYNC_READ | DMA_BUF_SYNC_WRITE)
#define DMA_BUF_SYNC_START (0 << 2)
#define DMA_BUF_SYNC_END (1 << 2)
#define DMA_BUF_SYNC_VALID_FLAGS_MASK (DMA_BUF_SYNC_RW | DMA_BUF_SYNC_END)

/**
 * ion info
 */
struct ion_info {
	struct ion_allocation_data alloc_data;
	void *virt_addr;
};

#endif /* ION_HEAD__H */

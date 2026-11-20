/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Kernel interface of the Samsung dma-buf heaps for Exynos drivers that are
 * still on the Exynos IOVMM.
 *
 * SPDX-FileCopyrightText: The LineageOS Project
 */

#ifndef _LINUX_SAMSUNG_DMA_HEAP_H
#define _LINUX_SAMSUNG_DMA_HEAP_H

#include <linux/dma-buf.h>
#include <linux/err.h>
#include <linux/types.h>

struct dma_buf_attachment;

#if IS_ENABLED(CONFIG_DMABUF_SAMSUNG_HEAPS)
/**
 * samsung_dma_heap_alloc() - allocate a dma-buf from a heap by name
 * @name: registered heap name, e.g. "system", "system-uncached", "vframe-secure"
 * @len:  size in bytes
 */
struct dma_buf *samsung_dma_heap_alloc(const char *name, size_t len);

/**
 * samsung_dma_buf_cached() - whether the CPU maps @dmabuf cacheable
 *
 * Only buffers exported by the Samsung heaps can be cached, so anything
 * else returns false.
 */
bool samsung_dma_buf_cached(struct dma_buf *dmabuf);

/**
 * samsung_dma_buf_iova_map() - device virtual address of an attachment
 * @attachment: attachment of a Samsung heap buffer to a System MMU master
 *
 * Returns the address of the whole buffer in the System MMU domain of
 * attachment->dev, or a negative errno value (check with IS_ERR_VALUE()).
 * Protected buffers return their secure address. The address is shared by
 * every master of the domain and lives until the buffer is freed, the call
 * only takes a reference that samsung_dma_buf_iova_unmap() drops.
 */
dma_addr_t samsung_dma_buf_iova_map(struct dma_buf_attachment *attachment);
void samsung_dma_buf_iova_unmap(struct dma_buf_attachment *attachment,
				dma_addr_t iova);
#else
static inline struct dma_buf *samsung_dma_heap_alloc(const char *name,
						     size_t len)
{
	return ERR_PTR(-ENODEV);
}

static inline bool samsung_dma_buf_cached(struct dma_buf *dmabuf)
{
	return false;
}

static inline dma_addr_t samsung_dma_buf_iova_map(struct dma_buf_attachment *attachment)
{
	return -ENODEV;
}

static inline void samsung_dma_buf_iova_unmap(struct dma_buf_attachment *attachment,
					      dma_addr_t iova)
{
}
#endif

#endif /* _LINUX_SAMSUNG_DMA_HEAP_H */

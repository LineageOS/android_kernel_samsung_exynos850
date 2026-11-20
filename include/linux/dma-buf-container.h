/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Dma buf container header
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 */

#ifndef _LINUX_DMA_BUF_CONTAINER_H_
#define _LINUX_DMA_BUF_CONTAINER_H_

#include <linux/dma-buf.h>
#include <linux/err.h>

#if IS_ENABLED(CONFIG_DMABUF_CONTAINER)

int dmabuf_container_get_count(struct dma_buf *dmabuf);
struct dma_buf *dmabuf_container_get_buffer(struct dma_buf *dmabuf, int index);
int dmabuf_container_set_mask(struct dma_buf *dmabuf, u64 mask);
int dmabuf_container_get_mask(struct dma_buf *dmabuf, u64 *mask);
struct dma_buf *dma_buf_get_any(int fd);

#else

static inline int dmabuf_container_get_count(struct dma_buf *dmabuf)
{
	return -EINVAL;
}

static inline struct dma_buf *dmabuf_container_get_buffer(struct dma_buf *dbuf,
							  int index)
{
	return NULL;
}

static inline int dmabuf_container_set_mask(struct dma_buf *dmabuf, u64 mask)
{
	return -EINVAL;
}

static inline int dmabuf_container_get_mask(struct dma_buf *dmabuf, u64 *mask)
{
	return -EINVAL;
}

static inline struct dma_buf *dma_buf_get_any(int fd)
{
	return dma_buf_get(fd);
}

#endif

#endif /* _LINUX_DMA_BUF_CONTAINER_H_ */

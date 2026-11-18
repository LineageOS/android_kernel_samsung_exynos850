/* SPDX-License-Identifier: GPL-2.0 */
/*
 * SPDX-FileCopyrightText: The LineageOS Project
 *
 * Shadow call stack interworking with prebuilt code that uses x18 as a
 * scratch register. See arch/arm64/kernel/blob_x18_stubs.S.
 */
#ifndef __ASM_BLOB_X18_H
#define __ASM_BLOB_X18_H

#include <linux/types.h>

#ifdef CONFIG_SHADOW_CALL_STACK
/* Declare [start, end) as x18-clobbering prebuilt code */
void blob_x18_set_range(unsigned long start, unsigned long end);
/* Wrap a blob function pointer the kernel will call (no-op outside the range) */
void *blob_x18_k2b(void *fn);
/* Wrap a kernel function pointer the blob will call (no-op if not kernel text) */
void *blob_x18_b2k(void *fn);
/* Wrap every pointer-sized word of a table matching the above */
void blob_x18_wrap_k2b_table(void *table, size_t size);
void blob_x18_wrap_b2k_table(void *table, size_t size);
#else
static inline void blob_x18_set_range(unsigned long start, unsigned long end) {}
static inline void *blob_x18_k2b(void *fn) { return fn; }
static inline void *blob_x18_b2k(void *fn) { return fn; }
static inline void blob_x18_wrap_k2b_table(void *table, size_t size) {}
static inline void blob_x18_wrap_b2k_table(void *table, size_t size) {}
#endif

#endif /* __ASM_BLOB_X18_H */

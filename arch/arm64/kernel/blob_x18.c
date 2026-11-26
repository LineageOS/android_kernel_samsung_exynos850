// SPDX-License-Identifier: GPL-2.0
/*
 * SPDX-FileCopyrightText: The LineageOS Project
 *
 * Shadow call stack interworking with prebuilt code that uses x18 as a
 * scratch register. Hands out the stubs from blob_x18_stubs.S.
 */

#include <linux/bug.h>
#include <linux/compiler.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/spinlock.h>
#include <asm/blob_x18.h>

#define BLOB_X18_K2B_NR		256
#define BLOB_X18_B2K_NR		512
#define BLOB_X18_STUB_SIZE	16

extern unsigned long blob_x18_ranges[6];
extern unsigned long blob_x18_k2b_tgt[BLOB_X18_K2B_NR];
extern unsigned long blob_x18_b2k_tgt[BLOB_X18_B2K_NR];
extern char blob_x18_k2b_stubs[], blob_x18_b2k_stubs[];

struct blob_x18_pool {
	const char *name;
	unsigned long *tgt;
	char *stubs;
	unsigned int max;
	unsigned int used;
};

static struct blob_x18_pool k2b_pool = {
	.name	= "k2b",
	.tgt	= blob_x18_k2b_tgt,
	.stubs	= blob_x18_k2b_stubs,
	.max	= BLOB_X18_K2B_NR,
};

static struct blob_x18_pool b2k_pool = {
	.name	= "b2k",
	.tgt	= blob_x18_b2k_tgt,
	.stubs	= blob_x18_b2k_stubs,
	.max	= BLOB_X18_B2K_NR,
};

static DEFINE_RAW_SPINLOCK(blob_x18_lock);

static bool blob_x18_in_blob(unsigned long addr)
{
	return addr >= READ_ONCE(blob_x18_ranges[0]) &&
	       addr < READ_ONCE(blob_x18_ranges[1]);
}

static bool blob_x18_is_stub(unsigned long addr)
{
	unsigned long k2b = (unsigned long)blob_x18_k2b_stubs;
	unsigned long b2k = (unsigned long)blob_x18_b2k_stubs;

	return (addr >= k2b && addr < k2b + BLOB_X18_K2B_NR * BLOB_X18_STUB_SIZE) ||
	       (addr >= b2k && addr < b2k + BLOB_X18_B2K_NR * BLOB_X18_STUB_SIZE);
}

static void *blob_x18_get_stub(struct blob_x18_pool *pool, unsigned long fn)
{
	unsigned long flags;
	unsigned int i, used;
	void *stub = NULL;

	/* Entries are only ever appended: look up without the lock first */
	used = smp_load_acquire(&pool->used);
	for (i = 0; i < used; i++)
		if (READ_ONCE(pool->tgt[i]) == fn)
			return pool->stubs + i * BLOB_X18_STUB_SIZE;

	raw_spin_lock_irqsave(&blob_x18_lock, flags);
	for (i = 0; i < pool->used; i++) {
		if (pool->tgt[i] == fn) {
			stub = pool->stubs + i * BLOB_X18_STUB_SIZE;
			goto out;
		}
	}
	if (pool->used < pool->max) {
		WRITE_ONCE(pool->tgt[i], fn);
		/* publish the target before the lockless lookup can see it */
		smp_store_release(&pool->used, i + 1);
		stub = pool->stubs + i * BLOB_X18_STUB_SIZE;
	}
out:
	raw_spin_unlock_irqrestore(&blob_x18_lock, flags);

	if (!stub)
		panic("blob_x18: %s stubs exhausted (%u) for %pS\n",
		      pool->name, pool->max, (void *)fn);

	return stub;
}

void blob_x18_set_range(unsigned long start, unsigned long end)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&blob_x18_lock, flags);
	if (blob_x18_ranges[0] != start || blob_x18_ranges[1] != end) {
		/* Never expose a range that is larger than either value */
		/* order the updates against kernel_entry and the b2k stubs */
		WRITE_ONCE(blob_x18_ranges[1], 0);
		smp_wmb();	/* empty range before moving the start */
		WRITE_ONCE(blob_x18_ranges[0], start);
		smp_wmb();	/* new start before the new end */
		WRITE_ONCE(blob_x18_ranges[1], end);
		pr_info("blob_x18: x18 clobbering code at %#lx-%#lx\n", start, end);
	}
	raw_spin_unlock_irqrestore(&blob_x18_lock, flags);
}
EXPORT_SYMBOL_GPL(blob_x18_set_range);

void *blob_x18_k2b(void *fn)
{
	if (!fn || !blob_x18_in_blob((unsigned long)fn))
		return fn;

	return blob_x18_get_stub(&k2b_pool, (unsigned long)fn);
}
EXPORT_SYMBOL_GPL(blob_x18_k2b);

void *blob_x18_b2k(void *fn)
{
	unsigned long addr = (unsigned long)fn;

	if (!fn || blob_x18_is_stub(addr) || !core_kernel_text(addr))
		return fn;

	return blob_x18_get_stub(&b2k_pool, addr);
}
EXPORT_SYMBOL_GPL(blob_x18_b2k);

static bool blob_x18_is_used_stub(struct blob_x18_pool *pool, unsigned long addr)
{
	unsigned long start = (unsigned long)pool->stubs;

	return addr >= start &&
	       addr < start + smp_load_acquire(&pool->used) * BLOB_X18_STUB_SIZE;
}

bool blob_x18_cfi_target(unsigned long addr)
{
	return blob_x18_in_blob(addr) ||
	       blob_x18_is_used_stub(&k2b_pool, addr) ||
	       blob_x18_is_used_stub(&b2k_pool, addr);
}

void blob_x18_wrap_k2b_table(void *table, size_t size)
{
	void **p = table;
	size_t i;

	for (i = 0; i < size / sizeof(void *); i++)
		p[i] = blob_x18_k2b(p[i]);
}
EXPORT_SYMBOL_GPL(blob_x18_wrap_k2b_table);

void blob_x18_wrap_b2k_table(void *table, size_t size)
{
	void **p = table;
	size_t i;

	for (i = 0; i < size / sizeof(void *); i++)
		p[i] = blob_x18_b2k(p[i]);
}
EXPORT_SYMBOL_GPL(blob_x18_wrap_b2k_table);

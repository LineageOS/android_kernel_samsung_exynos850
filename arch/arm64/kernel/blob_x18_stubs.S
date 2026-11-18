/* SPDX-License-Identifier: GPL-2.0 */
/*
 * SPDX-FileCopyrightText: The LineageOS Project
 */

#include <linux/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/assembler.h>

#define BLOB_X18_K2B_NR		256
#define BLOB_X18_B2K_NR		512

	.pushsection .data, "aw"
	.align	3
ENTRY(blob_x18_ranges)
	.quad	0, 0					// blob code, set at runtime
	.quad	blob_x18_k2b_ret, blob_x18_k2b_ret_end
	.quad	blob_x18_b2k_stubs, blob_x18_b2k_end
END(blob_x18_ranges)
	.popsection

	.pushsection .bss, "aw"
	.align	3
ENTRY(blob_x18_k2b_tgt)
	.space	BLOB_X18_K2B_NR * 8
END(blob_x18_k2b_tgt)
ENTRY(blob_x18_b2k_tgt)
	.space	BLOB_X18_B2K_NR * 8
END(blob_x18_b2k_tgt)
	.popsection

	.text
	.align	4
ENTRY(blob_x18_k2b_stubs)
	.set	.Lk2b_i, 0
	.rept	BLOB_X18_K2B_NR
	adrp	x16, blob_x18_k2b_tgt + .Lk2b_i * 8
	ldr	x16, [x16, #:lo12:blob_x18_k2b_tgt + .Lk2b_i * 8]
	b	blob_x18_k2b_common
	nop
	.set	.Lk2b_i, .Lk2b_i + 1
	.endr
END(blob_x18_k2b_stubs)

	/* x16 = blob function, x18 = valid shadow stack */
blob_x18_k2b_common:
	str	x30, [x18], #8
	mrs	x17, sp_el0
	ldr	x30, [x17, #TSK_TI_BLOB_SCS]
	str	x30, [x18], #8
	str	x18, [x17, #TSK_TI_BLOB_SCS]
	adr	x30, blob_x18_k2b_ret
	br	x16

	/* x18 restored, x17 = current */
blob_x18_k2b_tail:
	ldr	x30, [x18, #-8]!
	str	x30, [x17, #TSK_TI_BLOB_SCS]
	ldr	x30, [x18, #-8]!
	ret

	/* The blob returns here with x18 clobbered */
blob_x18_k2b_ret:
	mrs	x17, sp_el0
	ldr	x18, [x17, #TSK_TI_BLOB_SCS]
	b	blob_x18_k2b_tail
blob_x18_k2b_ret_end:

	.align	4
ENTRY(blob_x18_b2k_stubs)
	.set	.Lb2k_i, 0
	.rept	BLOB_X18_B2K_NR
	adrp	x16, blob_x18_b2k_tgt + .Lb2k_i * 8
	ldr	x16, [x16, #:lo12:blob_x18_b2k_tgt + .Lb2k_i * 8]
	b	blob_x18_b2k_common
	nop
	.set	.Lb2k_i, .Lb2k_i + 1
	.endr
END(blob_x18_b2k_stubs)

	/* x16 = kernel function, x18 = garbage if called from a blob */
blob_x18_b2k_common:
	stp	x0, x1, [sp, #-16]!
	adr_l	x0, blob_x18_ranges
	ldp	x1, x17, [x0]
	cmp	x30, x1
	b.lo	1f
	cmp	x30, x17
	b.lo	2f
1:	ldp	x1, x17, [x0, #16]
	cmp	x30, x1
	b.lo	3f
	cmp	x30, x17
	b.hs	3f
2:	ldp	x0, x1, [sp], #16
	mrs	x17, sp_el0
	ldr	x18, [x17, #TSK_TI_BLOB_SCS]
	br	x16
3:	ldp	x0, x1, [sp], #16
	br	x16
blob_x18_b2k_end:

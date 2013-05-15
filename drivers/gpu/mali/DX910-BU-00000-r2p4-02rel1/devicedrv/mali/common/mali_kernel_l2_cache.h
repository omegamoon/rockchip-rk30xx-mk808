/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __MALI_KERNEL_L2_CACHE_H__
#define __MALI_KERNEL_L2_CACHE_H__

#include "mali_osk.h"
#include "mali_kernel_subsystem.h"
extern struct mali_kernel_subsystem mali_subsystem_l2_cache;

_mali_osk_errcode_t mali_kernel_l2_cache_invalidate_all(void);
_mali_osk_errcode_t mali_kernel_l2_cache_invalidate_page(u32 page);

void mali_kernel_l2_cache_do_enable(void);
void mali_kernel_l2_cache_set_perf_counters(u32 src0, u32 src1, int force_reset);
void mali_kernel_l2_cache_get_perf_counters(u32 *src0, u32 *val0, u32 *src1, u32 *val1);

#endif /* __MALI_KERNEL_L2_CACHE_H__ */

/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2009-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __UMP_KERNEL_TYPES_H__
#define __UMP_KERNEL_TYPES_H__

#include "ump_kernel_interface.h"
#include "mali_osk.h"

/*
 * This struct is what is "behind" a ump_dd_handle
 */
typedef struct ump_dd_mem
{
	ump_secure_id secure_id;
	_mali_osk_atomic_t ref_count;
	unsigned long size_bytes;
	unsigned long nr_blocks;
	ump_dd_physical_block * block_array;
	void (*release_func)(void * ctx, struct ump_dd_mem * descriptor);
	void * ctx;
	void * backend_info;
	int is_cached;
} ump_dd_mem;



#endif /* __UMP_KERNEL_TYPES_H__ */

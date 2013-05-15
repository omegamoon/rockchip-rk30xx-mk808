/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2009-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file ump_internal.c
 *
 * Internal definitions and debugging macros for the UMP implementation.
 */

#ifndef _UNIFIED_MEMORY_PROVIDER_INTERNAL_H_
#define _UNIFIED_MEMORY_PROVIDER_INTERNAL_H_

#include <ump/ump.h>
#include <ump/ump_osu.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ump_cache_enabled
{
	UMP_CACHE_DISABLE = 0,
	UMP_CACHE_ENABLE  = 1
} ump_cache_enabled;

/**
 * The actual (hidden) definition of ump_handles.
 */
typedef struct ump_mem
{
	ump_secure_id secure_id;       /**< UMP device driver cookie */
	void * mapped_mem;             /**< Mapped memory; all read and write use this */
	unsigned long size;            /**< Size of allocated memory */
	_ump_osu_lock_t* ref_lock;    /**< Lock protection ref_count */
	int ref_count;                 /**< The reference count of the ump_handle in userspace. It is used for finding out
	                                    when to free the memory used by this userspace handle. It is NOT the same as the
	                                    real ump_mem reference count in the devicedriver which do reference counting
	                                    for the memory that this handle reveals. */
	unsigned long cookie;          /**< cookie for use in arch_unmap calls */
	ump_cache_enabled is_cached;
} ump_mem;

#ifdef __cplusplus
}
#endif



#endif /* _UNIFIED_MEMORY_PROVIDER_INTERNAL_H_ */

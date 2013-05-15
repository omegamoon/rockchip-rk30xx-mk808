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
 * @file ump_arch.c
 *
 * UMP arch layer for UMP-UDD
 */

#include <ump/ump.h>
#include "ump_arch.h"
#include <ump/ump_debug.h>

#include <ump/ump_uk_types.h>
#include "../os/ump_uku.h"

/** Pointer to an OS-Specific context that we should pass in _uku_ calls */
void *ump_uk_ctx = NULL;

/** Reference counting of ump_arch_open() and ump_arch_close(). */
static volatile int ump_ref_count = 0;

/** Lock for critical section in open/close */
_ump_osu_lock_t * ump_lock = NULL;

ump_result ump_arch_open(void)
{
	ump_result retval = UMP_OK;

	_ump_osu_lock_auto_init( &ump_lock, 0, 0, 0 );

	/* Check that the lock was initialized */
	if (NULL == ump_lock)
	{
		UMP_DEBUG_PRINT(1, ("UMP: ump_arch_open() failed to init lock\n"));
		return UMP_ERROR;
	}

	/* Attempt to obtain a lock */
	if( _UMP_OSU_ERR_OK !=  _ump_osu_lock_wait( ump_lock, _UMP_OSU_LOCKMODE_RW ) )
	{
		UMP_DEBUG_PRINT(1, ("UMP: ump_arch_open() failed to acquire lock\n"));
		return UMP_ERROR;
	}

	/* ASSERT NEEDED */
	UMP_DEBUG_ASSERT(0 <= ump_ref_count, ("UMP: Reference count invalid at _ump_base_arch_open()"));
	ump_ref_count++;

	if (1 == ump_ref_count)
	{
		/* We are the first, open the UMP device driver */

		if (_UMP_OSU_ERR_OK != _ump_uku_open( &ump_uk_ctx ))
		{
			UMP_DEBUG_PRINT(1, ("UMP: ump_arch_open() failed to open UMP device driver\n"));
			retval = UMP_ERROR;
			ump_ref_count--;
		}
	}

	/* Signal the lock so someone else can use it */
	 _ump_osu_lock_signal( ump_lock, _UMP_OSU_LOCKMODE_RW );

	return retval;
}



void ump_arch_close(void)
{
	_ump_osu_lock_auto_init( &ump_lock, 0, 0, 0 );

	/* Check that the lock was initialized */
	if(NULL == ump_lock)
	{
		UMP_DEBUG_PRINT(1, ("UMP: ump_arch_close() failed to init lock\n"));
		return;
	}

	/* Attempt to obtain a lock */
	if( _UMP_OSU_ERR_OK !=  _ump_osu_lock_wait( ump_lock, _UMP_OSU_LOCKMODE_RW ) )
	{
		UMP_DEBUG_PRINT(1, ("UMP: ump_arch_close() failed to acquire lock\n"));
		return;
	}

	UMP_DEBUG_ASSERT(0 < ump_ref_count, ("UMP: ump_arch_close() called while no references exist"));
	if (ump_ref_count > 0)
	{
		ump_ref_count--;
		if (0 == ump_ref_count)
		{
			_ump_osu_errcode_t retval = _ump_uku_close(&ump_uk_ctx);
			UMP_DEBUG_ASSERT(retval == _UMP_OSU_ERR_OK, ("UMP: Failed to close UMP interface"));
			UMP_IGNORE(retval);
			ump_uk_ctx = NULL;
			_ump_osu_lock_signal( ump_lock, _UMP_OSU_LOCKMODE_RW );
			_ump_osu_lock_term( ump_lock ); /* Not 100% thread safe, since another thread can already be waiting for this lock in ump_arch_open() */
			ump_lock = NULL;
			return;
		}
	}

	/* Signal the lock so someone else can use it */
	 _ump_osu_lock_signal( ump_lock, _UMP_OSU_LOCKMODE_RW );
}



ump_secure_id ump_arch_allocate(unsigned long * size, ump_alloc_constraints constraints)
{
	_ump_uk_allocate_s call_arg;

	if ( NULL == size )
	{
		return UMP_INVALID_SECURE_ID;
	}

	call_arg.ctx = ump_uk_ctx;
	call_arg.secure_id = UMP_INVALID_SECURE_ID;
	call_arg.size = *size;
#ifdef UMP_DEBUG_SKIP_CODE
	/** Run-time ASSERTing that _ump_uk_api_version_s and ump_alloc_constraints are
	 * interchangable */
	switch (constraints)
	{
	case UMP_REF_DRV_CONSTRAINT_NONE:
		UMP_DEBUG_ASSERT( UMP_REF_DRV_UK_CONSTRAINT_NONE == constraints, ("ump_uk_alloc_constraints out of sync with ump_alloc_constraints") );
		break;
	case UMP_REF_DRV_CONSTRAINT_PHYSICALLY_LINEAR:
		UMP_DEBUG_ASSERT( UMP_REF_DRV_UK_CONSTRAINT_PHYSICALLY_LINEAR == constraints, ("ump_uk_alloc_constraints out of sync with ump_alloc_constraints") );
		break;
	default:
		UMP_DEBUG_ASSERT( 1, ("ump_uk_alloc_constraints out of sync with ump_alloc_constraints: %d unrecognized", constraints) );
		break;
	}
#endif
	call_arg.constraints = (ump_uk_alloc_constraints)constraints;

	if ( _UMP_OSU_ERR_OK != _ump_uku_allocate(&call_arg) )
	{
		return UMP_INVALID_SECURE_ID;
	}

	*size = call_arg.size;

	UMP_DEBUG_PRINT(4, ("UMP: Allocated ID %u, size %ul", call_arg.secure_id, call_arg.size));

	return call_arg.secure_id;
}



unsigned long ump_arch_size_get(ump_secure_id secure_id)
{
	_ump_uk_size_get_s dd_size_call_arg;

	dd_size_call_arg.ctx = ump_uk_ctx;
	dd_size_call_arg.secure_id = secure_id;
	dd_size_call_arg.size = 0;

	if (_UMP_OSU_ERR_OK == _ump_uku_size_get( &dd_size_call_arg ) )
	{
		return dd_size_call_arg.size;
	}

	return 0;
}


void ump_arch_reference_release(ump_secure_id secure_id)
{
	_ump_uk_release_s dd_release_call_arg;
	_ump_osu_errcode_t retval;

	dd_release_call_arg.ctx = ump_uk_ctx;
	dd_release_call_arg.secure_id = secure_id;

	UMP_DEBUG_PRINT(4, ("UMP: Releasing ID %u", secure_id));

	retval = _ump_uku_release( &dd_release_call_arg );
	UMP_DEBUG_ASSERT(retval == _UMP_OSU_ERR_OK, ("UMP: Failed to release reference to UMP memory"));
	UMP_IGNORE(retval);
}


void* ump_arch_map(ump_secure_id secure_id, unsigned long size, ump_cache_enabled cache, unsigned long *cookie_out)
{
	_ump_uk_map_mem_s dd_map_call_arg;

	UMP_DEBUG_ASSERT_POINTER( cookie_out );

	dd_map_call_arg.ctx = ump_uk_ctx;
	dd_map_call_arg.secure_id = secure_id;
	dd_map_call_arg.size = size;
	dd_map_call_arg.is_cached = (u32) (UMP_CACHE_ENABLE==cache);

	if ( -1 == _ump_uku_map_mem( &dd_map_call_arg ) )
	{
		UMP_DEBUG_PRINT(4, ("UMP: Mapping failed for ID %u", secure_id));
		return NULL;
	}

	UMP_DEBUG_PRINT(4, ("Mapped %u at 0x%08lx", secure_id, (unsigned long)dd_map_call_arg.mapping));

	*cookie_out = dd_map_call_arg.cookie;
	return dd_map_call_arg.mapping;
}



void ump_arch_unmap(void* mapping, unsigned long size, unsigned long cookie)
{
	_ump_uk_unmap_mem_s dd_unmap_call_arg;

	dd_unmap_call_arg.ctx = ump_uk_ctx;
	dd_unmap_call_arg.mapping = mapping;
	dd_unmap_call_arg.size = size;
	dd_unmap_call_arg.cookie = cookie;

	UMP_DEBUG_PRINT(4, ("Unmapping 0x%08lx", (unsigned long)mapping));
	_ump_uku_unmap_mem( &dd_unmap_call_arg );
}

/** Memory synchronization - cache flushing of mapped memory */
int ump_arch_msync(ump_secure_id secure_id, void* mapping, unsigned long cookie, void * address, unsigned long size,  ump_cpu_msync_op op)
{
	_ump_uk_msync_s dd_msync_call_arg;

	dd_msync_call_arg.ctx = ump_uk_ctx;
	dd_msync_call_arg.mapping = mapping;
	dd_msync_call_arg.address = address;
	dd_msync_call_arg.size = size;
	dd_msync_call_arg.op = (ump_uk_msync_op)op;
	dd_msync_call_arg.cookie = cookie;
	dd_msync_call_arg.secure_id = secure_id;
	dd_msync_call_arg.is_cached = 0;

	UMP_DEBUG_PRINT(4, ("Msync 0x%08lx", (unsigned long)mapping));
	_ump_uku_msynch( &dd_msync_call_arg );
	if ( 0==dd_msync_call_arg.is_cached )
	{
		UMP_DEBUG_PRINT(4, ("Trying to flush uncached UMP mem ID: %d", secure_id));
	}
	return dd_msync_call_arg.is_cached;
}

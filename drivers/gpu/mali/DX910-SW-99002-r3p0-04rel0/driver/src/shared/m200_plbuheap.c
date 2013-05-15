/*
 * This confidential and proprietary software may be used only as
 * authorized by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2011-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#include "m200_plbuheap.h"

#if !defined(HARDWARE_ISSUE_3251)
/* default heap size must be 1024 aligned as this is the maximum tile list block bytesize */
#ifndef __SYMBIAN32__
#define MALI_PLBUHEAP_SIZE_INIT (1024 * 192)    /* The initial size of a PLBU heap */
#define MALI_PLBUHEAP_SIZE_GROW (1024 * 256)    /* The increment on a PLBU heap*/
#else
/* Symbian devices prefer to conserve memory, and also make less use of these heaps in practice */
#define MALI_PLBUHEAP_SIZE_INIT (1024 * 64)
#define MALI_PLBUHEAP_SIZE_GROW (1024 * 128)
#endif
#define MALI_PLBUHEAP_SIZE_MAX  (1024 * 1024 *16 * 4)

#else
/* use one large static heap block */
#define MALI_PLBUHEAP_SIZE_STATIC (1024 * 1024 * 8)
#endif

MALI_CHECK_RESULT mali_plbuheap* _mali_plbuheap_alloc( mali_base_ctx_handle base_ctx, void* owner )
{
	u32 i;
	mali_plbuheap* heap = NULL;

	heap = _mali_sys_calloc(1, sizeof( mali_plbuheap ) );
	if(heap == NULL) return NULL;

	heap->plbu_ds_resource = mali_ds_resource_allocate( base_ctx, owner, NULL );
	if( MALI_NO_HANDLE == heap->plbu_ds_resource )
	{
		_mali_sys_free( heap );
		return NULL;
	}

	_mali_sys_atomic_initialize(&heap->use_count, 0);

#if !defined(HARDWARE_ISSUE_3251)
	heap->init_size = (u32)_mali_sys_config_string_get_s64("MALI_FRAME_HEAP_SIZE", MALI_PLBUHEAP_SIZE_INIT, 4096, MALI_PLBUHEAP_SIZE_MAX);
	heap->plbu_heap = _mali_mem_heap_alloc( base_ctx, heap->init_size , MALI_PLBUHEAP_SIZE_MAX, MALI_PLBUHEAP_SIZE_GROW );
#else
	heap->init_size = (u32)_mali_sys_config_string_get_s64("MALI_FRAME_HEAP_SIZE", MALI_PLBUHEAP_SIZE_STATIC, 4096, MALI_PLBUHEAP_SIZE_STATIC);
	heap->plbu_heap = _mali_mem_alloc( base_ctx, heap_size, 1024, MALI_PP_READ | MALI_GP_WRITE );
#endif
	
	if( MALI_NO_HANDLE == heap->plbu_heap )
	{
		_mali_plbuheap_free( heap );
		return NULL;
	}

	return heap;
}

void _mali_plbuheap_free( mali_plbuheap* heap )
{
	MALI_DEBUG_ASSERT_POINTER( heap );
	MALI_DEBUG_ASSERT_POINTER( heap->plbu_heap );
	MALI_DEBUG_ASSERT_HANDLE( heap->plbu_ds_resource );
	MALI_DEBUG_ASSERT( _mali_sys_atomic_get( &heap->use_count) == 0, ("Cannot free PLBU heap while it is still in use") );

	if(heap->plbu_heap) _mali_mem_free( heap->plbu_heap );
	if(heap->plbu_ds_resource) mali_ds_resource_release_connections( heap->plbu_ds_resource, MALI_DS_RELEASE, MALI_DS_ABORT_ALL );
	_mali_sys_free( heap );
}

void _mali_plbuheap_add_usecount(mali_plbuheap* heap)
{
	MALI_DEBUG_ASSERT_POINTER( heap );
	MALI_DEBUG_ASSERT_HANDLE( heap->plbu_heap );
	
	_mali_sys_atomic_inc(&heap->use_count);
}

void _mali_plbuheap_dec_usecount(mali_plbuheap* heap)
{
	MALI_DEBUG_ASSERT_POINTER( heap );
	MALI_DEBUG_ASSERT_HANDLE( heap->plbu_heap );
	
	if( _mali_sys_atomic_dec_and_return(&heap->use_count) == 0 )
	{
		#if !defined(HARDWARE_ISSUE_3251)
		if(heap->reset) 
		{
			heap->reset = MALI_FALSE;
			_mali_mem_heap_reset( heap->plbu_heap );
		}
		#endif
	}
}

void _mali_plbuheap_reset( mali_plbuheap* heap )
{
#if !defined(HARDWARE_ISSUE_3251)

	MALI_DEBUG_ASSERT_POINTER( heap );
	MALI_DEBUG_ASSERT_HANDLE( heap->plbu_heap );

	heap->reset = MALI_TRUE;
	if( _mali_sys_atomic_get( &heap->use_count) > 0 )
	{
		_mali_mem_heap_reset( heap->plbu_heap );
		heap->reset = MALI_FALSE;
	}
#endif
}

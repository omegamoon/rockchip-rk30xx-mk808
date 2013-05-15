/*
 * Copyright (C) 2011-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _M200_PLBUHEAP_H_
#define _M200_PLBUHEAP_H_

#include <mali_system.h>
#include <base/mali_memory.h>
#include <base/mali_dependency_system.h>

/**
 * This file provides a structure holding all information about the PLBU heap.  
 *
 * In its simplest form, the PLBU heap is simply a memory block. 
 *
 */

struct mali_plbuheap
{
	/* ALL of these fields are READ ONLY outside of the setters provided by this file. Do not modify! */

	mali_mem_handle plbu_heap;                  /**< The actual PLBU heap memory. */
	u32  init_size;
	u32  last_used_sizes[4];
	mali_bool reset;                            /**< Set when a reset is needed but couldn't happen due to the PLBU heap being in use */
	mali_atomic_int use_count;                  /**< Count the number of times this has gone through a job. Zero after a reset */
	mali_ds_resource_handle plbu_ds_resource;   /**< The DS resource associated with this memory block. Used to prevent concurrent access. */

};

/**
 * Allocate a new PLBU heap. 
 * The heap will be allocated holding MALI_PLBUHEAP_SIZE_INIT number of bytes. 
 *
 * @param base_ctx            The current base context 
 * @param owner               for frame_builder
 * @param bHwLimitation       set when it is only for the hw limitation case
 * @return                    A new PLBU heap object
 */
MALI_CHECK_RESULT mali_plbuheap* _mali_plbuheap_alloc( mali_base_ctx_handle base_ctx, void* owner, mali_bool bHwLimitation );

/**
 * Free an allocated PLBU heap
 * This function will assert that the usecount is 0 when freeing. 
 *
 * This function will free the heap object unquestionably. 
 *
 * @param heap     The heap to free  
 */
void _mali_plbuheap_free( mali_plbuheap* heap );

/**
 * Add the use_count when heap is in use.
 *
 * @param heap     The heap used 
*/
void _mali_plbuheap_add_usecount(mali_plbuheap* heap);

/**
 * Decrease the use_count when heap is no longer in use.
 *
 * @param heap     The heap used
*/
void _mali_plbuheap_dec_usecount(mali_plbuheap* heap);

/**
 * Resets the PLBU heap back down to the size given when it was allocated. 
 * If the PLBU heap has been grown one or more times, this function will 
 * free the "grown" memory. This operation is faster than
 * just reallocating the plbuheap structure. 
 *
 * If the PLBU heap is in use within a frame, this operation will be deferred. 
 * Calling this function while the heap is in use is thus safe. 
 *
 * Side note: 
 * HARDWARE_ISSUE_3251 prevents setting the end register of PLBU 
 * heaps from the GPU. Resizing and as a result, resetting the heap 
 * can as such not happen. If this issue is present, the function does nothing.  
 *
 * @param heap     The heap to reset. 
 *
 */
void _mali_plbuheap_reset(mali_plbuheap* heap);


#endif /* _M200_PLBUHEAP_H_ */


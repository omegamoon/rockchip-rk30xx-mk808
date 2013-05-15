/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2010, 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __MALI_KERNEL_MEM_OS_H__
#define __MALI_KERNEL_MEM_OS_H__

/**
 * @brief Creates an object that manages allocating OS memory
 *
 * Creates an object that provides an interface to allocate OS memory and
 * have it mapped into the Mali virtual memory space.
 *
 * The object exposes pointers to
 * - allocate OS memory
 * - allocate Mali page tables in OS memory
 * - destroy the object
 *
 * Allocations from OS memory are of type mali_physical_memory_allocation
 * which provides a function to release the allocation.
 *
 * @param max_allocation max. number of bytes that can be allocated from OS memory
 * @param cpu_usage_adjust value to add to mali physical addresses to obtain CPU physical addresses
 * @param name description of the allocator
 * @return pointer to mali_physical_memory_allocator object. NULL on failure.
 **/
mali_physical_memory_allocator * mali_os_allocator_create(u32 max_allocation, u32 cpu_usage_adjust, const char *name);

#endif /* __MALI_KERNEL_MEM_OS_H__ */



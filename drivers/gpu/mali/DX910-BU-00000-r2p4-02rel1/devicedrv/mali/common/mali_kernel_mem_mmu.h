/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __MALI_KERNEL_MEM_MMU_H__
#define __MALI_KERNEL_MEM_MMU_H__

#include "mali_kernel_session_manager.h"

/**
 * Lookup a MMU core by ID.
 * @param id ID of the MMU to find
 * @return NULL if ID not found or valid, non-NULL if a core was found.
 */
void* mali_memory_core_mmu_lookup(u32 id);

/**
 * Set the core pointer of MMU to core owner of MMU
 *
 * @param core Core holding this MMU
 * @param mmu_ptr The MMU whose core pointer needs set to core holding the MMU
 * 
 */
void mali_memory_core_mmu_owner(void *core, void *mmu_ptr);

/**
 * Activate a user session with its address space on the given MMU.
 * If the session can't be activated due to that the MMU is busy and
 * a callback pointer is given, the callback will be called once the MMU becomes idle.
 * If the same callback pointer is registered multiple time it will only be called once.
 * Nested activations are supported.
 * Each call must be matched by a call to @see mali_memory_core_mmu_release_address_space_reference
 *
 * @param mmu The MMU to activate the address space on
 * @param mali_session_data The user session object which address space to activate
 * @param callback Pointer to the function to call when the MMU becomes idle
 * @param callback_arg Argument given to the callback
 * @return 0 if the address space was activated, -EBUSY if the MMU was busy, -EFAULT in all other cases.
 */
int mali_memory_core_mmu_activate_page_table(void* mmu_ptr, struct mali_session_data * mali_session_data, void(*callback)(void*), void * callback_argument);

/**
 * Release a reference to the current active address space.
 * Once the last reference is released any callback(s) registered will be called before the function returns
 *
 * @note Caution must be shown calling this function with locks held due to that callback can be called
 * @param mmu The mmu to release a reference to the active address space of
 */
void mali_memory_core_mmu_release_address_space_reference(void* mmu);

/**
 * Soft reset of MMU - needed after power up
 *
 * @param mmu_ptr The MMU pointer registered with the relevant core
 */
void mali_kernel_mmu_reset(void * mmu_ptr);

void mali_kernel_mmu_force_bus_reset(void * mmu_ptr);

/**
 * Unregister a previously registered callback.
 * @param mmu The MMU to unregister the callback on
 * @param callback The function to unregister
 */
void mali_memory_core_mmu_unregister_callback(void* mmu, void(*callback)(void*));



#endif /* __MALI_KERNEL_MEM_MMU_H__ */

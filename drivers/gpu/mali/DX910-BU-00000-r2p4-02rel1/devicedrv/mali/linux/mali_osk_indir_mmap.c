/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2010, 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/sched.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/atomic.h>

#include "mali_osk.h"
#include "mali_ukk.h"
#include "mali_kernel_common.h"

/**
 * @file mali_osk_specific.c
 * Implementation of per-OS Kernel level specifics
 */

_mali_osk_errcode_t _mali_osk_specific_indirect_mmap( _mali_uk_mem_mmap_s *args )
{
	/* args->ctx ignored here; args->ukk_private required instead */
	/* we need to lock the mmap semaphore before calling the do_mmap function */
    down_write(&current->mm->mmap_sem);

    args->mapping = (void __user *)do_mmap(
											(struct file *)args->ukk_private,
											0, /* start mapping from any address after NULL */
											args->size,
											PROT_READ | PROT_WRITE,
											MAP_SHARED,
											args->phys_addr
										   );

	/* and unlock it after the call */
	up_write(&current->mm->mmap_sem);

	/* No cookie required here */
	args->cookie = 0;
	/* uku_private meaningless, so zero */
	args->uku_private = NULL;

	if ( (NULL == args->mapping) || IS_ERR((void *)args->mapping) )
	{
		return _MALI_OSK_ERR_FAULT;
	}

	/* Success */
	return _MALI_OSK_ERR_OK;
}


_mali_osk_errcode_t _mali_osk_specific_indirect_munmap( _mali_uk_mem_munmap_s *args )
{
	/* args->ctx and args->cookie ignored here */

	if ((NULL != current) && (NULL != current->mm))
	{
		/* remove mapping of mali memory from the process' view */
		/* lock mmap semaphore before call */
		/* lock mmap_sem before calling do_munmap */
		down_write(&current->mm->mmap_sem);
		do_munmap(
					current->mm,
					(unsigned long)args->mapping,
					args->size
				);
		/* and unlock after call */
		up_write(&current->mm->mmap_sem);
		MALI_DEBUG_PRINT(5, ("unmapped\n"));
	}
	else
	{
		MALI_DEBUG_PRINT(2, ("Freeing of a big block while no user process attached, assuming crash cleanup in progress\n"));
	}

	return _MALI_OSK_ERR_OK; /* always succeeds */
}

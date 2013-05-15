/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file mali_osk_specific.h
 * Defines per-OS Kernel level specifics, such as unusual workarounds for
 * certain OSs.
 */

#ifndef __MALI_OSK_INDIR_MMAP_H__
#define __MALI_OSK_INDIR_MMAP_H__

#include "mali_uk_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Linux specific means for calling _mali_ukk_mem_mmap/munmap
 *
 * The presence of _MALI_OSK_SPECIFIC_INDIRECT_MMAP indicates that
 * _mali_osk_specific_indirect_mmap and _mali_osk_specific_indirect_munmap
 * should be used instead of _mali_ukk_mem_mmap/_mali_ukk_mem_munmap.
 *
 * The arguments are the same as _mali_ukk_mem_mmap/_mali_ukk_mem_munmap.
 *
 * In ALL operating system other than Linux, it is expected that common code
 * should be able to call _mali_ukk_mem_mmap/_mali_ukk_mem_munmap directly.
 * Such systems should NOT define _MALI_OSK_SPECIFIC_INDIRECT_MMAP.
 */
_mali_osk_errcode_t _mali_osk_specific_indirect_mmap( _mali_uk_mem_mmap_s *args );
_mali_osk_errcode_t _mali_osk_specific_indirect_munmap( _mali_uk_mem_munmap_s *args );


#ifdef __cplusplus
}
#endif

#endif /* __MALI_OSK_INDIR_MMAP_H__ */

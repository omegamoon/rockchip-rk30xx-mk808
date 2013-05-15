/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2010, 2012 ARM Limited
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

#ifndef __MALI_OSK_SPECIFIC_H__
#define __MALI_OSK_SPECIFIC_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define MALI_STATIC_INLINE     static inline
#define MALI_NON_STATIC_INLINE inline

#ifdef __cplusplus
}
#endif

#endif /* __MALI_OSK_SPECIFIC_H__ */

/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2010, 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __MALI_DEVICE_PAUSE_RESUME_H__
#define __MALI_DEVICE_PAUSE_RESUME_H__

#if USING_MALI_PMM
int mali_dev_pause(void);
int mali_dev_resume(void);
#endif /* USING_MALI_PMM */

#endif /* __MALI_DEVICE_PAUSE_RESUME_H__ */

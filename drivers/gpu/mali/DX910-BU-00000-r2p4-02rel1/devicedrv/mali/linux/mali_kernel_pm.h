/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2010, 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __MALI_KERNEL_PM_H__
#define __MALI_KERNEL_PM_H__

#ifdef USING_MALI_PMM
int _mali_dev_platform_register(void);
void _mali_dev_platform_unregister(void);
#endif /* USING_MALI_PMM */

#endif /* __MALI_KERNEL_PM_H__ */

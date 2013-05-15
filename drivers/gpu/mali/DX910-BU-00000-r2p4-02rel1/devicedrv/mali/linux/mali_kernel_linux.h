/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __MALI_KERNEL_LINUX_H__
#define __MALI_KERNEL_LINUX_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <linux/cdev.h>     /* character device definitions */
#include "mali_kernel_license.h"
#include "mali_osk.h"

struct mali_dev
{
	struct cdev cdev;
#if MALI_LICENSE_IS_GPL
	struct class *  mali_class;
#endif
};

#if MALI_LICENSE_IS_GPL
/* Defined in mali_osk_irq.h */
extern struct workqueue_struct * mali_wq;
#endif

_mali_osk_errcode_t initialize_kernel_device(void);
void terminate_kernel_device(void);

void mali_osk_low_level_mem_init(void);
void mali_osk_low_level_mem_term(void);

#ifdef __cplusplus
}
#endif

#endif /* __MALI_KERNEL_LINUX_H__ */

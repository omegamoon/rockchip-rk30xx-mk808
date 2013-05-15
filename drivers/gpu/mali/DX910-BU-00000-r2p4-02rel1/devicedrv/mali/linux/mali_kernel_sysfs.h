/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2011-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __MALI_KERNEL_SYSFS_H__
#define __MALI_KERNEL_SYSFS_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define MALI_PROC_DIR "driver/mali"

int mali_sysfs_register(struct mali_dev *mali_class, dev_t dev, const char *mali_dev_name);

int mali_sysfs_unregister(struct mali_dev *mali_class, dev_t dev, const char *mali_dev_name);


#ifdef __cplusplus
}
#endif

#endif /* __MALI_KERNEL_LINUX_H__ */

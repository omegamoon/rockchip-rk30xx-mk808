/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2010-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */
#include <linux/fs.h>       /* file system operations */
#include <asm/uaccess.h>    /* user space access */

#include "mali_ukk.h"
#include "mali_osk.h"
#include "mali_kernel_common.h"
#include "mali_kernel_session_manager.h"
#include "mali_ukk_wrappers.h"

int profiling_start_wrapper(struct mali_session_data *session_data, _mali_uk_profiling_start_s __user *uargs)
{
	_mali_uk_profiling_start_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);

	if (0 != copy_from_user(&kargs, uargs, sizeof(_mali_uk_profiling_start_s)))
	{
		return -EFAULT;
	}

	kargs.ctx = session_data;
	err = _mali_ukk_profiling_start(&kargs);
	if (_MALI_OSK_ERR_OK != err)
	{
		return map_errcode(err);
	}

	if (0 != put_user(kargs.limit, &uargs->limit))
	{
		return -EFAULT;
	}

	return 0;
}

int profiling_add_event_wrapper(struct mali_session_data *session_data, _mali_uk_profiling_add_event_s __user *uargs)
{
	_mali_uk_profiling_add_event_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);

	if (0 != copy_from_user(&kargs, uargs, sizeof(_mali_uk_profiling_add_event_s)))
	{
		return -EFAULT;
	}

	kargs.ctx = session_data;
	err = _mali_ukk_profiling_add_event(&kargs);
	if (_MALI_OSK_ERR_OK != err)
	{
		return map_errcode(err);
	}

	return 0;
}

int profiling_stop_wrapper(struct mali_session_data *session_data, _mali_uk_profiling_stop_s __user *uargs)
{
	_mali_uk_profiling_stop_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);

	kargs.ctx = session_data;
	err = _mali_ukk_profiling_stop(&kargs);
	if (_MALI_OSK_ERR_OK != err)
	{
		return map_errcode(err);
	}

	if (0 != put_user(kargs.count, &uargs->count))
	{
		return -EFAULT;
	}

	return 0;
}

int profiling_get_event_wrapper(struct mali_session_data *session_data, _mali_uk_profiling_get_event_s __user *uargs)
{
	_mali_uk_profiling_get_event_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);

	if (0 != get_user(kargs.index, &uargs->index))
	{
		return -EFAULT;
	}

	kargs.ctx = session_data;

	err = _mali_ukk_profiling_get_event(&kargs);
	if (_MALI_OSK_ERR_OK != err)
	{
		return map_errcode(err);
	}

	kargs.ctx = NULL; /* prevent kernel address to be returned to user space */
	if (0 != copy_to_user(uargs, &kargs, sizeof(_mali_uk_profiling_get_event_s)))
	{
		return -EFAULT;
	}

	return 0;
}

int profiling_clear_wrapper(struct mali_session_data *session_data, _mali_uk_profiling_clear_s __user *uargs)
{
	_mali_uk_profiling_clear_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);

	kargs.ctx = session_data;
	err = _mali_ukk_profiling_clear(&kargs);
	if (_MALI_OSK_ERR_OK != err)
	{
		return map_errcode(err);
	}

	return 0;
}

int profiling_get_config_wrapper(struct mali_session_data *session_data, _mali_uk_profiling_get_config_s __user *uargs)
{
	_mali_uk_profiling_get_config_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);

	kargs.ctx = session_data;
	err = _mali_ukk_profiling_get_config(&kargs);
	if (_MALI_OSK_ERR_OK != err)
	{
		return map_errcode(err);
	}

	if (0 != put_user(kargs.enable_events, &uargs->enable_events))
	{
		return -EFAULT;
	}

	return 0;
}


/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2010-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __MALI_KERNEL_UTILIZATION_H__
#define __MALI_KERNEL_UTILIZATION_H__

#include "mali_osk.h"

/**
 * Initialize/start the Mali GPU utilization metrics reporting.
 *
 * @return _MALI_OSK_ERR_OK on success, otherwise failure.
 */
_mali_osk_errcode_t mali_utilization_init(void);

/**
 * Terminate the Mali GPU utilization metrics reporting
 */
void mali_utilization_term(void);

/**
 * Should be called when a job is about to execute a job
 */
void mali_utilization_core_start(u64 time_now);

/**
 * Should be called to stop the utilization timer during system suspend
 */
void mali_utilization_suspend(void);

/**
 * Should be called when a job has completed executing a job
 */
void mali_utilization_core_end(u64 time_now);


#endif /* __MALI_KERNEL_UTILIZATION_H__ */

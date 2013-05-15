/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2010-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */
#ifndef __MALI_LINUX_PM_TESTSUITE_H__
#define __MALI_LINUX_PM_TESTSUITE_H__

#if USING_MALI_PMM
#if MALI_POWER_MGMT_TEST_SUITE
#ifdef CONFIG_PM

typedef enum
{
        _MALI_DEVICE_PMM_TIMEOUT_EVENT,
        _MALI_DEVICE_PMM_JOB_SCHEDULING_EVENTS,
	_MALI_DEVICE_PMM_REGISTERED_CORES,
        _MALI_DEVICE_MAX_PMM_EVENTS

} _mali_device_pmm_recording_events;

extern unsigned int mali_timeout_event_recording_on;
extern unsigned int mali_job_scheduling_events_recording_on;
extern unsigned int pwr_mgmt_status_reg;
extern unsigned int is_mali_pmm_testsuite_enabled;
extern unsigned int is_mali_pmu_present;

#endif /* CONFIG_PM */
#endif /* MALI_POWER_MGMT_TEST_SUITE */
#endif /* USING_MALI_PMM */
#endif /* __MALI_LINUX_PM_TESTSUITE_H__ */



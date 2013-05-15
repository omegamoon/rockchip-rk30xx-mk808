/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2009-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file mali_platform.h
 * Platform specific Mali driver functions
 */

#include "mali_osk.h"

#if !USING_MALI_PMM
/* @brief System power up/down cores that can be passed into mali_platform_powerdown/up() */
#define MALI_PLATFORM_SYSTEM  0
#endif

#if USING_MALI_PMM
#if USING_MALI_PMU
#include "mali_pmm.h"

/** @brief Platform specific setup and initialisation of MALI
 * 
 * This is called from the entrypoint of the driver to initialize the platform
 * When using PMM, it is also called from the PMM start up to initialise the 
 * system PMU
 *
 * @param resource This is NULL when called on first driver start up, else it will
 * be a pointer to a PMU resource
 * @return _MALI_OSK_ERR_OK on success otherwise, a suitable _mali_osk_errcode_t error.
 */
_mali_osk_errcode_t mali_pmm_pmu_init(_mali_osk_resource_t *resource);

/** @brief Platform specific deinitialisation of MALI
 * 
 * This is called on the exit of the driver to terminate the platform
 * When using PMM, it is also called from the PMM termination code to clean up the
 * system PMU
 *
 * @param type This is NULL when called on driver exit, else it will
 * be a pointer to a PMU resource type (not the full resource)
 * @return _MALI_OSK_ERR_OK on success otherwise, a suitable _mali_osk_errcode_t error.
 */
_mali_osk_errcode_t mali_pmm_pmu_deinit(_mali_osk_resource_type_t *type);

/** @brief Platform specific powerdown sequence of MALI
 * 
 * Called as part of platform init if there is no PMM support, else the
 * PMM will call it.
 *
 * @param cores This is MALI_PLATFORM_SYSTEM when called without PMM, else it will
 * be a mask of cores to power down based on the mali_pmm_core_id enum
 * @return _MALI_OSK_ERR_OK on success otherwise, a suitable _mali_osk_errcode_t error.
 */
_mali_osk_errcode_t mali_pmm_pmu_powerdown(u32 cores);

/** @brief Platform specific powerup sequence of MALI
 * 
 * Called as part of platform deinit if there is no PMM support, else the
 * PMM will call it.
 *
 * @param cores This is MALI_PLATFORM_SYSTEM when called without PMM, else it will
 * be a mask of cores to power down based on the mali_pmm_core_id enum
 * @return _MALI_OSK_ERR_OK on success otherwise, a suitable _mali_osk_errcode_t error.
 */
_mali_osk_errcode_t mali_pmm_pmu_powerup(u32 cores);

#if MALI_POWER_MGMT_TEST_SUITE
#if USING_MALI_PMM
#if USING_MALI_PMU
/** @brief function to get status of individual cores
 *
 * This function is used by power management test suite to get the status of powered up/down the number
 * of cores
 * @param utilization The workload utilization of the Mali GPU. 0 = no utilization, 256 = full utilization.
 */
u32 pmu_get_power_up_down_info(void);
#endif
#endif
#endif
#endif
#endif

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
 * @file mali_pmm_system.h
 * Defines the power management module system functions
 */

#ifndef __MALI_PMM_SYSTEM_H__
#define __MALI_PMM_SYSTEM_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup pmmapi Power Management Module APIs
 *
 * @{
 *
 * @defgroup pmmapi_system Power Management Module System Functions
 *
 * @{
 */

extern struct mali_kernel_subsystem mali_subsystem_pmm;

/** @brief Register a core with the PMM, which will power up
 * the core
 *
 * @param core the core to register with the PMM
 * @return error if the core cannot be powered up
 */
_mali_osk_errcode_t malipmm_core_register( mali_pmm_core_id core );

/** @brief Unregister a core with the PMM
 *
 * @param core the core to unregister with the PMM
 */
void malipmm_core_unregister( mali_pmm_core_id core );

/** @brief Acknowledge that a power down is okay to happen
 *
 * A core should not be running a job, or be in the idle queue when this
 * is called.
 *
 * @param core the core that can now be powered down
 */
void malipmm_core_power_down_okay( mali_pmm_core_id core );

/** @} */ /* End group pmmapi_system */
/** @} */ /* End group pmmapi */

#ifdef __cplusplus
}
#endif

#endif /* __MALI_PMM_H__ */

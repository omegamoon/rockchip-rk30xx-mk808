/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2010, 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file mali_pmm_policy_alwayson.h
 * Defines the power management module policy for always on
 */

#ifndef __MALI_PMM_POLICY_ALWAYSON_H__
#define __MALI_PMM_POLICY_ALWAYSON_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup pmmapi_policy Power Management Module Policies
 *
 * @{
 */

/** @brief Always on policy initialization
 *
 * @return _MALI_OSK_ERR_OK if the policy could be initialized, or a suitable
 * _mali_osk_errcode_t otherwise.
 */
_mali_osk_errcode_t pmm_policy_init_always_on(void);

/** @brief Always on policy termination
 */
void pmm_policy_term_always_on(void);

/** @brief Always on policy state changer
 *
 * Given the next available event message, this routine processes it
 * for the policy and changes state as needed.
 *
 * Always on policy will ignore all events and keep the Mali cores on
 * all the time
 *
 * @param pmm internal PMM state
 * @param event PMM event to process
 * @return _MALI_OSK_ERR_OK if the policy state completed okay, or a suitable
 * _mali_osk_errcode_t otherwise.
 */
_mali_osk_errcode_t pmm_policy_process_always_on( _mali_pmm_internal_state_t *pmm, mali_pmm_message_t *event );

/** @} */ /* End group pmmapi_policies */

#ifdef __cplusplus
}
#endif

#endif /* __MALI_PMM_POLICY_ALWAYSON_H__ */

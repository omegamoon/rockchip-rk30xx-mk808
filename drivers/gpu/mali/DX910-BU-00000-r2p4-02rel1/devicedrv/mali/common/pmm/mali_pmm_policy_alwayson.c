/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2010-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file mali_pmm_policy_alwayson.c
 * Implementation of the power management module policy - always on
 */

#if USING_MALI_PMM

#include "mali_ukk.h"
#include "mali_kernel_common.h"

#include "mali_pmm.h"
#include "mali_pmm_system.h"
#include "mali_pmm_state.h"
#include "mali_pmm_policy_alwayson.h"

_mali_osk_errcode_t pmm_policy_init_always_on(void)
{
	/* Nothing to set up */
	MALI_SUCCESS;
}

void pmm_policy_term_always_on(void)
{
	/* Nothing to tear down */
}

_mali_osk_errcode_t pmm_policy_process_always_on( _mali_pmm_internal_state_t *pmm, mali_pmm_message_t *event )
{
	MALI_DEBUG_ASSERT_POINTER(pmm);
	MALI_DEBUG_ASSERT_POINTER(event);

	switch( event->id )
	{
		case MALI_PMM_EVENT_OS_POWER_DOWN:
			/* We aren't going to do anything, but signal so we don't block the OS
			 * NOTE: This may adversely affect any jobs Mali is currently running
			 */
			_mali_osk_pmm_power_down_done( event->data );
		break;

		case MALI_PMM_EVENT_INTERNAL_POWER_UP_ACK:
		case MALI_PMM_EVENT_INTERNAL_POWER_DOWN_ACK:
			/* Not expected in this policy */
			MALI_DEBUG_ASSERT( MALI_FALSE );
		break;

		case MALI_PMM_EVENT_OS_POWER_UP:
			/* Nothing to do */
			_mali_osk_pmm_power_up_done( event->data );
		break;
		
		case MALI_PMM_EVENT_JOB_SCHEDULED:
		case MALI_PMM_EVENT_JOB_QUEUED:
		case MALI_PMM_EVENT_JOB_FINISHED:
			/* Nothing to do - we are always on */
		break;
	
		case MALI_PMM_EVENT_TIMEOUT:
			/* Not expected in this policy */
			MALI_DEBUG_ASSERT( MALI_FALSE );
		break;

		default:
		MALI_ERROR(_MALI_OSK_ERR_ITEM_NOT_FOUND);
	}

	MALI_SUCCESS;
}

#endif

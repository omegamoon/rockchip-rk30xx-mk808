/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file mali_osk_mali.c
 * Implementation of the OS abstraction layer which is specific for the Mali kernel device driver
 */
#include <linux/kernel.h>
#include <asm/uaccess.h>

#include "mali_kernel_common.h" /* MALI_xxx macros */
#include "mali_osk.h"           /* kernel side OS functions */
#include "mali_uk_types.h"
#include "mali_kernel_linux.h"  /* exports initialize/terminate_kernel_device() definition of mali_osk_low_level_mem_init() and term */
#include "arch/config.h"        /* contains the configuration of the arch we are compiling for */

/* is called from mali_kernel_constructor in common code */
_mali_osk_errcode_t _mali_osk_init( void )
{
	if (0 != initialize_kernel_device()) MALI_ERROR(_MALI_OSK_ERR_FAULT);

	mali_osk_low_level_mem_init();
	
	MALI_SUCCESS;
}

/* is called from mali_kernel_deconstructor in common code */
void _mali_osk_term( void )
{
	mali_osk_low_level_mem_term();
	terminate_kernel_device();
}

_mali_osk_errcode_t _mali_osk_resources_init( _mali_osk_resource_t **arch_config, u32 *num_resources )
{
    *num_resources = sizeof(arch_configuration) / sizeof(arch_configuration[0]);
    *arch_config = arch_configuration;
    return _MALI_OSK_ERR_OK;
}

void _mali_osk_resources_term( _mali_osk_resource_t **arch_config, u32 num_resources )
{
    /* Nothing to do */
}

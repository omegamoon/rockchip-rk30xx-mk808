/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2010, 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __ARCH_CONFIG_H__
#define __ARCH_CONFIG_H__

/* Configuration for the Versatile Express platform */

#define MALI_BASE_ADDRESS 0xFC010000

static _mali_osk_resource_t arch_configuration [] =
{
	{
		.type = MALI400GP,
		.description = "Mali-400 GP",
		.base = MALI_BASE_ADDRESS,
		.irq = -1,
		.mmu_id = 1
	},
	{
		.type = MALI400PP,
		.base = MALI_BASE_ADDRESS + 0x8000,
		.irq = -1,
		.description = "Mali-400 PP",
		.mmu_id = 2
	},
	{
		.type = MMU,
		.base = MALI_BASE_ADDRESS + 0x3000,
		.irq = -1,
		.description = "Mali-400 MMU for GP",
		.mmu_id = 1
	},
	{
		.type = MMU,
		.base = MALI_BASE_ADDRESS + 0x4000,
		.irq = -1,
		.description = "Mali-400 MMU for PP",
		.mmu_id = 2
	},
	{
		.type = OS_MEMORY,
		.description = "Mali OS memory",
		.cpu_usage_adjust = 0,
		.alloc_order = 0, /* Highest preference for this memory */
		.base = 0x0,
		.size = 256 * 1024 * 1024,
		.flags = _MALI_CPU_WRITEABLE | _MALI_CPU_READABLE | _MALI_PP_READABLE | _MALI_PP_WRITEABLE |_MALI_GP_READABLE | _MALI_GP_WRITEABLE
	},
	{
		.type = MEM_VALIDATION,
		.description = "Framebuffer",
		.base = 0xe0000000,
		.size = 0x01000000,
		.flags = _MALI_CPU_WRITEABLE | _MALI_CPU_READABLE | _MALI_PP_WRITEABLE | _MALI_PP_READABLE
	},
	{
		.type = MALI400L2,
		.base = MALI_BASE_ADDRESS + 0x1000,
		.description = "Mali-400 L2 cache"
	},
};

#endif /* __ARCH_CONFIG_H__ */

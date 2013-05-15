/*
 * Copyright (C) 2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <shared/mali_sw_counters.h>
#include <base/mali_profiling.h>

MALI_EXPORT struct mali_sw_counters* _mali_sw_counters_alloc()
{
	/* just return an NULL'ed memory block. 
	 * No special initialization needed. */
	return _mali_sys_calloc( 1, sizeof( mali_sw_counters ) );
}


MALI_EXPORT void _mali_sw_counters_free(struct mali_sw_counters* counters)
{
	_mali_sys_free(counters);
}

MALI_EXPORT void _mali_sw_counters_dump(struct mali_sw_counters* counters)
{
	u32* counter_list;

	MALI_DEBUG_ASSERT_POINTER(counters);

	counter_list = counters->counter;

	_mali_base_profiling_report_sw_counters(counter_list, sizeof(counters->counter) / sizeof(counters->counter[0]));

}


MALI_EXPORT void _mali_sw_counters_reset(struct mali_sw_counters* counters)
{
	MALI_DEBUG_ASSERT_POINTER(counters);
	_mali_sys_memset( counters, 0, sizeof( mali_sw_counters ) );
}



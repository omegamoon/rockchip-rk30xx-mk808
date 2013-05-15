/*
 * Copyright (C) 2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _MALI_SW_COUNTERS_H_
#define _MALI_SW_COUNTERS_H_

#include <mali_system.h>
#include <mali_utgard_counters.h>

/* this file should only do something if SW counters are enabled */
#if MALI_SW_COUNTERS_ENABLED


/* Work out the number of counters in each group */
#define MALI_PROFILING_N_EGL_COUNTERS  (MALI_CINSTR_EGL_MAX_COUNTER - MALI_CINSTR_COUNTER_SOURCE_EGL)
#define MALI_PROFILING_N_GLES_COUNTERS (MALI_CINSTR_GLES_MAX_COUNTER - MALI_CINSTR_COUNTER_SOURCE_OPENGLES)
#define MALI_PROFILING_N_VG_COUNTERS   (MALI_CINSTR_VG_MAX_COUNTER - MALI_CINSTR_COUNTER_SOURCE_OPENVG)

/* The total number of software counters in all groups (used to size buffers) */
#define MALI_PROFILING_N_SW_COUNTERS   (MALI_PROFILING_N_EGL_COUNTERS + MALI_PROFILING_N_GLES_COUNTERS + MALI_PROFILING_N_VG_COUNTERS)

/* Work out the offset of each group from the base of the buffer */
#define MALI_PROFILING_EGL_INDEX_BASE  (0)
#define MALI_PROFILING_GLES_INDEX_BASE (MALI_PROFILING_N_EGL_COUNTERS)
#define MALI_PROFILING_VG_INDEX_BASE   (MALI_PROFILING_GLES_INDEX_BASE + MALI_PROFILING_N_GLES_COUNTERS)

/* Get the index of the named counter in the counter value table */
#define MALI_PROFILING_EGL_COUNTER_INDEX(name)  (name - MALI_CINSTR_COUNTER_SOURCE_EGL + MALI_PROFILING_EGL_INDEX_BASE)
#define MALI_PROFILING_GLES_COUNTER_INDEX(name) (name - MALI_CINSTR_COUNTER_SOURCE_OPENGLES + MALI_PROFILING_GLES_INDEX_BASE)
#define MALI_PROFILING_VG_COUNTER_INDEX(name)   (name - MALI_CINSTR_COUNTER_SOURCE_OPENVG + MALI_PROFILING_VG_INDEX_BASE)

/*
 * Macro which increments the value associated with a specific counter.
 */
#define MALI_PROFILING_INCREMENT_COUNTER(counters, base, name, increment) do { \
	(counters)->counter[MALI_PROFILING_##base##_COUNTER_INDEX(MALI_CINSTR_##base##_##name)] += (increment);} while(0)

/*
 * Structure which holds s/w counter data on a framebuilder object.  These structures are created
 * when the framebuilder is created.  Counter values are indexed into this array using the macros
 * outlined above.
 */
typedef struct mali_sw_counters
{
    u32 counter[MALI_PROFILING_N_SW_COUNTERS]; /* Values for each s/w counter */
} mali_sw_counters;

/**
 * Allocates the structure containing all the software counters
 * @return A legal pointer, or NULL on error. 
 */
struct mali_sw_counters* _mali_sw_counters_alloc(void);


/**
 * Frees the structure containing information about software counters.
 * @param counters - The struct to free
 */
void _mali_sw_counters_free(struct mali_sw_counters* counters);

/*
 * Sends software counters to gator.
 * This function is used as a callback function assigned to 
 * the end of any frame using SW counters. 
 *
 * @param counters - The counters to report. 
 *
 */
void _mali_sw_counters_dump(struct mali_sw_counters* counters);

/**
 * Reset the SW counter struct, nulling all counters
 * @param counters - The counters to reset
 */
void _mali_sw_counters_reset(struct mali_sw_counters* counters);

#endif /* MALI_SW_COUNTERS_ENABLED */
#endif /* _MALI_SW_COUNTERS_H_ */

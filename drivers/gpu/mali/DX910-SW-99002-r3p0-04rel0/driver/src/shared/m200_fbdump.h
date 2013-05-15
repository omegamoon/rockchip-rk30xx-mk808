/*
 * Copyright (C) 2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef M200_FBDUMP_H
#define M200_FBDUMP_H

#include <mali_system.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Frame builder surface and buffer data can change while the frame is being built.
 * This structure holds the original references to those items.
 */
typedef struct
{
	struct mali_surface        *surface;
	struct mali_shared_mem_ref *mem_ref;    /**< Texture data in a mali memory block */
	u32                        mem_offset;  /**< Offset in mali memory block where the texture data starts. */
} mali_surface_info_type;

/**
 * Check whether to dump the next frame (checks the rate, surface, etc).
 * @param fbuilder A pointer to the frame builder to set the callback.
 *
 * @return MALI_TRUE if the frame should be dumped.
 */
mali_bool _mali_fbdump_is_requested(mali_frame_builder* fbuilder);

/**
 * Callback function that writes the requested frame buffer to the annotation channel.
 *
 * @param surface_info A struct that contains the references to the memory and the surface to be dumped.
 */
void _mali_fbdump_dump_callback(mali_surface_info_type* surface_info);

/**
 * Setup fbdumps callbacks on the current frame in the given framebuilder.
 * The callbacks will happen once the frame is done rendering
 *
 * @param frame_builder The framebuilder to set up callbacks on
 * @return MALI_ERR_NO_ERROR on success, or MALI_ERR_OUT_OF_MEMORY on failure 
 */
mali_err_code _mali_fbdump_setup_callbacks( struct mali_frame_builder* frame_builder);

#ifdef __cplusplus
}
#endif

#endif /* M200_FBDUMP_H */

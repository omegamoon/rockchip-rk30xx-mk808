/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _M200_PROJOB_H_
#define _M200_PROJOB_H_

#include <mali_system.h>
#include <base/mali_memory.h>
#include <regs/MALI200/mali_pp_cmd.h>

/* The projob struct. 
 * Holds all relevant data for a projob
 *
 * A projob carries a set of drawcalls, each needing to be flushed
 * before the normal job is executed. The max number of drawcalls 
 * you can fit into a screen is technically 4096x4096 if each drawcall 
 * is to cover a pixel on its own. I've settled for a limit of 16x16 
 * - that is one tile. This simplifies the setup code slightly as 
 * we only need to start one tile worth of rendering. Increasing 
 * PROJOB_DRAWCALL_LIMIT beyond one tile will require some 
 * setup changes below, but it's not too hard. 
 *
 * We do not want a limit of the number of projob drawcalls you can add 
 * to a frame though, as that effectively limits the number of drawcalls 
 * in a frame. To avoid this limit, we may need to set up several HW jobs
 * to cover all the projob drawcalls needed. 
 *
 * The projob structure holds an ever-expanding set of projob HW jobs, 
 * where one is the "current" one. As we fill up a projob, we leave said HW 
 * job in the array of unflushed HW jobs, and just work on filling up the
 * next one. 
 *
 * When we actually flush the HW job, we flush the entire array of 
 * outstanding projobs. Due to how the HW job structures work, by flushing them
 * you are actually giving them over to the base subsystem. After flushing,
 * their pointers are invalid, and you can/should never touch them again. 
 * Effectively, from this module's point of view, flushing the array of HW 
 * jobs means deleting them. 
 *
 */
struct mali_projob
{
	/* This is an array of all PP jobs needed to be flushed. 
	 * Grown as needed. Upon adding the first drawcall, there
	 * will be one job in this array. After adding more than 
	 * PROJOB_DRAWCALL_LIMIT drawcalls, there will be two jobs
	 * and so on. This array is submitted on flush. */
	mali_pp_job_handle* projob_pp_job_array;
	u32 projob_pp_job_array_size;

	/* After allocating a current PP job, the PP job will 
	 * need to output to a memory buffer. This memory buffer
	 * is allocated on the owner framebuilder frame pool,
	 * but it's the same memory block for all drawcalls within
	 * a PP job. */
	u64*       mapped_pp_tilelist;
	u32        mapped_pp_tilelist_bytesize;
	u32        mapped_pp_tilelist_byteoffset;
    mali_addr  pp_tilelist_addr;
    mali_addr  output_mali_addr;

	u32 num_pp_drawcalls_added_to_the_current_pp_job;

	/* the sync handle is allocated on the first draw
	 * and lasts until the next flush or reset. */
	mali_sync_handle sync_handle;
	mali_frame_cb_func cb_func;
	mali_frame_cb_param cb_param;


};

/* forward declaration */
struct mali_internal_frame;

/**
 * Add a pp drawcall to the projob struct
 *
 * The function will add one draw command to the master tile list of the projob. 
 * On flushing the projob, the drawcall will output to a FP16 vec4 memory area. 
 * The memory area the new drawcall is writing to is given by the return parameter. 
 *
 * @param fbuilder    - The framebuilder to add a projobdrawcall to
 * @param rsw_address - The RSW address defining this projob. Caller need to set this up!
 *                      Caller is also responsible that the lifetime of this memory is longer
 *                      than the time until this projob is done pp flushing. 
 * @return            - A legal mali_addr, or 0 on OOM. 
 *
 * NOTE: The framebuilder must be writelocked before calling this function.
 */
mali_addr _mali_projob_add_pp_drawcall(struct mali_frame_builder* fbuilder, mali_addr rsw_address);

/**
 * Flush the projob's pp drawcalls. 
 *
 * Submit the pp projob to HW immediately. All pp drawcalls added to the 
 * projob will be flushed as one HW job. 
 *
 * Drawcalls added to a projob will only be flushed once. All projob drawcalls are
 * reset after a flush. A previously given drawcall will only be flushed once, flushing the 
 * same fbuilder several times will have no effect the second time around. There is no 
 * "frame resume" feature for projobs, this call always starts and resets the projob drawcalls. 
 *
 * The projob drawcalls will be submitted to HW immediately, no depsystem will hold them back. 
 * It is the responsibility of the caller to ensure that all dependencies are met. 
 * Typically this function is called from within the framebuilder's activation step. 
 *
 * @param iframe - The frame to PP flush all projobs from
 * @return       - 1 if flushed, 0 if not flushed. 
 */
void _mali_projob_pp_flush(struct mali_internal_frame* frame);

/**
 * Reset the projob's drawcalls in the given frame. 
 *
 * This command will remove all pending drawcalls from the projob
 * in the given frame. The function is supposed to be called from within 
 * _mali_frame_builder_reset, when removing all drawcalls from 
 * a regular frame.
 *
 * @param iframe - The frame to remove all projobs from
 */
void _mali_projob_reset(struct mali_internal_frame* frame);

/**
 * Check if any _mali_projob_add_pp_drawcall() calls have been issued
 * since the last flush or reset. 
 *
 * @param frame      - The frame on which projob drawcalls may have been issued
 * @return mali_bool - TRUE or FALSE. 
 **/
mali_bool _mali_projob_have_drawcalls(struct mali_internal_frame* frame);

/**
 * Set up a callback that is executed when the next flush is completed. 
 * The callback state lasts until the frame is reset 
 * 
 * @param frame    - The frame on which to set up a callback 
 * @param cb_func  - The callback function to execute once all projobs are done
 * @param cb_param - The parameter to the cb_func
 **/
void _mali_projob_set_flush_callback(struct mali_internal_frame* frame, mali_frame_cb_func cb_func, mali_frame_cb_param cb_param);


#endif /* _M200_PROJOB_H_ */


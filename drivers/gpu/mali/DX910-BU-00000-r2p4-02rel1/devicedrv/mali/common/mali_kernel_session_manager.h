/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2010, 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __MALI_KERNEL_SESSION_MANAGER_H__
#define __MALI_KERNEL_SESSION_MANAGER_H__

/* Incomplete struct to pass around pointers to it */
struct mali_session_data;

void * mali_kernel_session_manager_slot_get(struct mali_session_data * session, int id);

#endif /* __MALI_KERNEL_SESSION_MANAGER_H__ */

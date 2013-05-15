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
 * @file ump_uku.h
 * Defines the user-side interface of the user-kernel interface
 */

#ifndef __UMP_UKU_H__
#define __UMP_UKU_H__

#include <ump/ump_osu.h>
#include <ump/ump_debug.h>
#include <ump/ump_uk_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

_ump_osu_errcode_t _ump_uku_open( void **context );

_ump_osu_errcode_t _ump_uku_close( void **context );

_ump_osu_errcode_t _ump_uku_allocate( _ump_uk_allocate_s *args );

_ump_osu_errcode_t _ump_uku_release( _ump_uk_release_s *args );

_ump_osu_errcode_t _ump_uku_size_get( _ump_uk_size_get_s *args );

_ump_osu_errcode_t _ump_uku_get_api_version( _ump_uk_api_version_s *args );

int _ump_uku_map_mem( _ump_uk_map_mem_s *args );

void _ump_uku_unmap_mem( _ump_uk_unmap_mem_s *args );

void _ump_uku_msynch(_ump_uk_msync_s *args);

#ifdef __cplusplus
}
#endif

#endif /* __UMP_UKU_H__ */

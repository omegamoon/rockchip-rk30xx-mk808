/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef MIDDLE_REWRITE_IMAGE_SAMPLER_ACCESSES_H
#define MIDDLE_REWRITE_IMAGE_SAMPLER_ACCESSES_H


#include "common/essl_mem.h"
#include "common/essl_target.h"
#include "common/basic_block.h"
memerr _essl_rewrite_sampler_accesses(mempool *pool, typestorage_context *ts_ctx, control_flow_graph *cfg);

#endif /* MIDDLE_REWRITE_IMAGE_SAMPLER_ACCESSES_H */


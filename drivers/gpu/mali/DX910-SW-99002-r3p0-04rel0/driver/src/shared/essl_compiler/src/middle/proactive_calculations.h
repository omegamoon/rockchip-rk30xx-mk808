/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MIDDLE_PROACTIVE_CALCULATIONS
#define MIDDLE_PROACTIVE_CALCULATIONS

#include "common/essl_target.h"
#include "common/translation_unit.h"
#include "common/essl_mem.h"

memerr _essl_optimise_constant_input_calculations(mempool *pool, typestorage_context *typestor_ctx, translation_unit* tu);

#endif 

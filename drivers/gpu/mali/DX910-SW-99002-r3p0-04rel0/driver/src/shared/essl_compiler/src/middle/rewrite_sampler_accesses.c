/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "common/essl_system.h"
#include "middle/rewrite_sampler_accesses.h"

typedef struct 
{
	mempool *pool;
	control_flow_graph *cfg;
	ptrset visited;
	typestorage_context *typestor_ctx;
}rewrite_sampler_accesses_context;

static memerr process_node(rewrite_sampler_accesses_context *ctx, node *n, essl_bool change_type)
{
	if(_essl_ptrset_has(&ctx->visited, n))
	{
		return MEM_OK;
	}

	if(_essl_node_is_texture_operation(n))
	{
		node *sampler;
		node *swz;

		assert(change_type == ESSL_FALSE);
		ESSL_CHECK(sampler = GET_CHILD(n, 0));
		ESSL_CHECK(swz = _essl_new_unary_expression(ctx->pool, EXPR_OP_SWIZZLE, sampler));
		_essl_ensure_compatible_node(swz, sampler);
		swz->expr.u.swizzle = _essl_create_scalar_swizzle(0);
		SET_CHILD(n, 0, swz);
		ESSL_CHECK(process_node(ctx, sampler, ESSL_TRUE));
	}

	if(change_type == ESSL_TRUE)
	{
		if(n->hdr.kind == EXPR_KIND_LOAD && 
			_essl_type_is_or_has_sampler(n->hdr.type))
		{
			ESSL_CHECK(n->hdr.type = _essl_get_type_with_given_vec_size(ctx->typestor_ctx, n->hdr.type, 4));
			ESSL_CHECK(_essl_ptrset_insert(&ctx->visited, n));
			return MEM_OK;
		}
		if (n->hdr.kind == EXPR_KIND_PHI)
		{
			phi_source *src;
			for (src = n->expr.u.phi.sources; src != 0; src = src->next)
			{
				ESSL_CHECK(process_node(ctx, src->source, change_type));
			}
		}else
		{
			unsigned i;
			for (i = 0; i < GET_N_CHILDREN(n); ++i)
			{
				node *child = GET_CHILD(n, i);

				ESSL_CHECK(process_node(ctx, child, change_type));
			}
		}

		ESSL_CHECK(n->hdr.type = _essl_get_type_with_given_vec_size(ctx->typestor_ctx, n->hdr.type, 4));
	}

	ESSL_CHECK(_essl_ptrset_insert(&ctx->visited, n));

	return MEM_OK;
}


static memerr handle_block(rewrite_sampler_accesses_context *ctx, basic_block *b)
{
	control_dependent_operation *c_op;
	for(c_op = b->control_dependent_ops; c_op != 0; c_op = c_op->next)
	{
		ESSL_CHECK(process_node(ctx, c_op->op, ESSL_FALSE));
	}

	return MEM_OK;
}



memerr _essl_rewrite_sampler_accesses(mempool *pool, typestorage_context *ts_ctx, control_flow_graph *cfg)
{

	rewrite_sampler_accesses_context ctx;
	unsigned int i;

	mempool visit_pool;

	ESSL_CHECK(_essl_mempool_init(&visit_pool, 0, _essl_mempool_get_tracker(pool)));
	ctx.pool = pool;
	ctx.cfg = cfg;
	ctx.typestor_ctx = ts_ctx;

	if(!_essl_ptrset_init(&ctx.visited, &visit_pool)) goto error;
	for (i = 0 ; i < cfg->n_blocks ; i++) {
		if(!handle_block(&ctx, cfg->postorder_sequence[i])) goto error;
	}
	_essl_mempool_destroy(&visit_pool);
	return MEM_OK;
error:
	_essl_mempool_destroy(&visit_pool);
	return MEM_ERROR;
}

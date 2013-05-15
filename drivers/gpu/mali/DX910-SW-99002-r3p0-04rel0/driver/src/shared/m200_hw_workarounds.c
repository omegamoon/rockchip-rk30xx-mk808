/*
 * This confidential and proprietary software may be used only as
 * authorized by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2006-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#include <mali_system.h>
#include "m200_gp_frame_builder_struct.h"

#if HARDWARE_ISSUE_4126
MALI_STATIC mali_mem_handle _mali_frame_builder_create_nop_shader(mali_base_ctx_handle base_ctx, int instructions)
{
	int i;
	const int _mali_gp_shader_alignment = 8;
	const u8 _mali_gp_nop_instruction[] =
	{
#if !MALI_BIG_ENDIAN
		0x08, 0x21, 0x04, 0x42,
		0x08, 0x01, 0x80, 0x03,
		0xe0, 0xff, 0x07, 0x00,
		0x00, 0x21, 0x04, 0x00
#else
/* We believe this is the correct big-endian form */
#error "This has not been tested"
		0x42, 0x04, 0x21, 0x08,
		0x03, 0x80, 0x01, 0x08,
		0x00, 0x07, 0xff, 0xe0,
		0x00, 0x04, 0x21, 0x00
#endif
};


	/* allocate memref */
	mali_mem_handle mem = _mali_mem_alloc(base_ctx, sizeof(_mali_gp_nop_instruction) * instructions,
					_mali_gp_shader_alignment, (mali_mem_usage_flag)(MALI_GP_READ | MALI_CPU_WRITE));
	if (NULL == mem) return NULL;

	/* write instructions */
	for (i = 0; i < instructions; ++i)
	{
		_mali_mem_write(
			mem,                                  /* dst */
			sizeof(_mali_gp_nop_instruction) * i, /* dst offset */
			(void*)_mali_gp_nop_instruction,      /* src */
			sizeof(_mali_gp_nop_instruction)      /* size */
		);
	}

	return mem;
}

MALI_EXPORT
mali_err_code _mali_frame_builder_workaround_for_bug_4126( mali_frame_builder* frame_builder, u32 num_vshader_instructions_in_next_drawcall)
{
	/* This command should be called after a use / write lock, which means the current frame is always available */
	mali_internal_frame *frame = GET_CURRENT_INTERNAL_FRAME( frame_builder );
	MALI_DEBUG_ASSERT_POINTER(frame);

	if( (u32) frame->last_vshader_instruction_count + 6 == num_vshader_instructions_in_next_drawcall)
	{
		mali_gp_job_handle gp_job;

/* (16 output + 16 input streams) */
#define NUM_REGISTERS (MALIGP2_MAX_VS_INPUT_REGISTERS + MALIGP2_MAX_VS_OUTPUT_REGISTERS)

/* ( 1 specifier + 1 address register for each stream), each 4 bytes */
#define NUM_REGISTERS_SIZE (NUM_REGISTERS*2*4)

		/* insert dummy GP job to avoid triggering bugzilla 4126 */
		int num_instructions = 1;
		mali_err_code err;
		u32 num_cmds = 0;
		u64 cmds[8];
		mali_mem_handle streams_mem;
		mali_mem_handle dummy_vshader;
		u32 dummy_streams[NUM_REGISTERS*2];
		int i;

		gp_job = _mali_frame_builder_get_gp_job( frame_builder );

		/* allocate empty streams memory, fill with "no stream" streams */
		streams_mem = _mali_mem_alloc(frame_builder->base_ctx, NUM_REGISTERS_SIZE, 64, MALI_PP_READ|MALI_GP_READ|MALI_GP_WRITE|MALI_CPU_WRITE);
		if(streams_mem == MALI_NO_HANDLE) return MALI_ERR_OUT_OF_MEMORY;

	    for (i = 0; i < MALIGP2_MAX_VS_INPUT_REGISTERS; i++)
		{
			dummy_streams[ GP_VS_CONF_REG_INP_ADDR( i ) ] = _SWAP_ENDIAN_U32_U32(0x0);
			dummy_streams[ GP_VS_CONF_REG_INP_SPEC( i ) ] = _SWAP_ENDIAN_U32_U32(GP_VS_VSTREAM_FORMAT_NO_DATA);
		}
	    for (i = 0; i < MALIGP2_MAX_VS_OUTPUT_REGISTERS; i++)
		{
			dummy_streams[ GP_VS_CONF_REG_OUTP_ADDR( i ) ] = _SWAP_ENDIAN_U32_U32(0x0);
			dummy_streams[ GP_VS_CONF_REG_OUTP_SPEC( i ) ] = _SWAP_ENDIAN_U32_U32(GP_VS_VSTREAM_FORMAT_NO_DATA);
		}

		_mali_mem_write(streams_mem, 0, dummy_streams, NUM_REGISTERS_SIZE);

		err = _mali_frame_builder_add_callback(
					frame_builder,
					MALI_STATIC_CAST(mali_frame_cb_func) _mali_mem_free,
					MALI_STATIC_CAST(mali_frame_cb_param) streams_mem
			);
		if (MALI_ERR_NO_ERROR != err)
		{
			_mali_mem_free(streams_mem);
			return err;
		}

		/* ensure that the dummy call don't trigger bugzilla 4126 itself */
		if ((u32) num_instructions + 6 == num_vshader_instructions_in_next_drawcall) num_instructions++;

		/* generate a dummy shader with 'num_instructions' instructions */
		dummy_vshader = _mali_frame_builder_create_nop_shader(frame_builder->base_ctx, num_instructions);
		if(dummy_vshader == NULL) return MALI_ERR_OUT_OF_MEMORY;

		/* Add the shader to the cleanup list so that it is deleted when the job ends */
		err = _mali_frame_builder_add_callback(
					frame_builder,
					MALI_STATIC_CAST(mali_frame_cb_func) _mali_mem_free,
					MALI_STATIC_CAST(mali_frame_cb_param) dummy_vshader
			);
		if (MALI_ERR_NO_ERROR != err)
		{
			_mali_mem_free(dummy_vshader);
			return err;
		}



		/* set up the dummy job */
		cmds[num_cmds++] = GP_VS_COMMAND_WRITE_INPUT_OUTPUT_CONF_REGS( _mali_mem_mali_addr_get(streams_mem, 0), 0, NUM_REGISTERS);
		cmds[num_cmds++] = GP_VS_COMMAND_LOAD_SHADER( _mali_mem_mali_addr_get(dummy_vshader, 0), 0, num_instructions);
		cmds[num_cmds++] = GP_VS_COMMAND_WRITE_CONF_REG( ((num_instructions - 1) << 10) | ((num_instructions - 1) << 20), GP_VS_CONF_REG_PROG_PARAM);
		cmds[num_cmds++] = GP_VS_COMMAND_WRITE_CONF_REG( GP_VS_CONF_REG_OPMOD_CREATE(1, 1), GP_VS_CONF_REG_OPMOD ); /* 1 input, 1 output */
		cmds[num_cmds++] = GP_VS_COMMAND_FLUSH_WRITEBACK_BUF();
		cmds[num_cmds++] = GP_VS_COMMAND_SHADE_VERTICES( MALI_TRUE, 1 ); /* fake-indices, one vertex */
#undef NUM_REGISTERS_SIZE
#undef NUM_REGISTERS

		MALI_DEBUG_ASSERT(num_cmds <= MALI_ARRAY_SIZE(cmds), ("commands written outside command buffer!"));

		/* add the dummy job */
		err = _mali_gp_job_add_vs_cmds(gp_job, cmds, num_cmds);
		if (MALI_ERR_NO_ERROR != err) return err;

	}

	frame->last_vshader_instruction_count = num_vshader_instructions_in_next_drawcall;
	return MALI_ERR_NO_ERROR;
}
#endif /* HW issue 4126 */



/*
 * This confidential and proprietary software may be used only as
 * authorized by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#include <mali_system.h>
#include <shared/m200_gp_frame_builder.h>
#include <shared/m200_gp_frame_builder_inlines.h>
#include <shared/m200_incremental_rendering.h>
#include <shared/mali_surface.h>
#include <shared/m200_fbdump.h>

typedef struct {
	u16 width;
	u16 height;
	u32 bpp;
	u32 mask_r;
	u32 mask_g;
	u32 mask_b;
	u32 mask_a;
} external_image_format;

/* Get the number of characters in a constant array (excluding the zero termination char). */
#define STRLEN(x) (sizeof(x) - 1)

/**
 * Generate bit masks per channel for some common formats that we support.
 * reverse_order is not supported.
 * @params mask_r, mask_g, mask_b, mask_a output parameters to hold channel bitmasks.
 * @return MALI_TRUE if the masks have been set.
 */
MALI_STATIC_INLINE mali_bool _mali_pixel_format_get_channel_mask(mali_surface_specifier *surface_format, u32 *mask_r, u32 *mask_g, u32 *mask_b, u32 *mask_a)
{
	MALI_DEBUG_ASSERT_POINTER(surface_format);
	MALI_DEBUG_ASSERT_POINTER(mask_r);
	MALI_DEBUG_ASSERT_POINTER(mask_g);
	MALI_DEBUG_ASSERT_POINTER(mask_b);
	MALI_DEBUG_ASSERT_POINTER(mask_a);

	switch (surface_format->pixel_format)
	{
	case MALI_PIXEL_FORMAT_R5G6B5:
		*mask_a = 0;
		*mask_r = 0xf800;
		*mask_g = 0x7E0;
		*mask_b = 0x1f;
		break;
	case MALI_PIXEL_FORMAT_A1R5G5B5:
		*mask_a = 0x8000;
		*mask_r = 0x7C00;
		*mask_g = 0x3E0;
		*mask_b = 0x1f;
		break;
	case MALI_PIXEL_FORMAT_A4R4G4B4:
		*mask_a = 0xf000;
		*mask_r = 0xf00;
		*mask_g = 0xf0;
		*mask_b = 0xf;
		break;
	case MALI_PIXEL_FORMAT_A8R8G8B8:
		*mask_a = 0xff000000;
		*mask_r = 0xff0000;
		*mask_g = 0xff00;
		*mask_b = 0xff;
		break;
	default:
		return MALI_FALSE;
	}

	if (surface_format->red_blue_swap)
	{
		u32 tmp = *mask_r;
		*mask_r = *mask_b;
		*mask_b = tmp;
	}

	return MALI_TRUE;
}

MALI_STATIC_INLINE void annotate_framebuffer(u8 *source_bytes, mali_surface_specifier *input_format, external_image_format *output_format, u32 resize_factor)
{
	u16 y;
	static const char *gator_str = "Framebuffer";
	static const char *image_type_id = "GPU1";
	const u32 size_of_external_image_format = sizeof(external_image_format);
	const u8 bytes_per_pixel = output_format->bpp / 8;
	const u16 pitch = input_format->pitch;
	const u32 n_bytes_in_image = bytes_per_pixel * output_format->width * output_format->height;

	/* Annotate header */
	const u32 length = _mali_sys_strlen(image_type_id) + sizeof(size_of_external_image_format)
					   + sizeof(external_image_format) + n_bytes_in_image;

	/* Gator header */
	long long visual_annotation = 0x011c | (_mali_sys_strlen(gator_str) << 16) | ((long long)length << 32);
	_mali_base_profiling_annotate_write(&visual_annotation, 8);
	_mali_base_profiling_annotate_write(gator_str, _mali_sys_strlen(gator_str));

	/* Image buffer header */
	_mali_base_profiling_annotate_write(image_type_id, _mali_sys_strlen(image_type_id));                                        /* Send a magic ID to identify the data */
	_mali_base_profiling_annotate_write(&size_of_external_image_format, sizeof(size_of_external_image_format));       /* Send the size of the following format structure, in bytes */

	/* Send the format data */
	_mali_base_profiling_annotate_write(output_format, sizeof(external_image_format));

	/* Now send the image data data */
	if (1 == resize_factor)
	{
		/* Send the full resolution buffer one line at a time */
		for(y = 0; y < output_format->height; ++y)
		{
			u32 line_offset = y * pitch;
			_mali_base_profiling_annotate_write(source_bytes + line_offset, bytes_per_pixel * output_format->width);
		}
	}
	else
	{
		/* Skip pixels to send a smaller image */
		for (y = 0; y < output_format->height; ++y)
		{
			u16 j;
			const u32 line_offset = y * resize_factor * pitch;

			/* Advance by 4 times resize_factor (usually 4 bytes per pixel) */
			for (j=0; j < output_format->width; ++j)
			{
				/* Send one pixel (usually 4 bytes) */
				_mali_base_profiling_annotate_write(source_bytes + line_offset + j * resize_factor * bytes_per_pixel, bytes_per_pixel);
			}
		}
	}
}

/*
 * Writes the content of the supplied surface to the visual stream for gator.
 * Data is written in a raw format with minimal header information for speed.  The Host is responsible for reassembly
 * of the image as it has more processing power available to it.
 */
MALI_STATIC void write_framebuffer_to_streamline(mali_surface_info_type *surface_info)
{
	const mali_bool setup_success = _mali_base_profiling_annotate_setup();

	/* Write visual annotation */
	if (MALI_TRUE == setup_success)
	{
		mali_surface_specifier *input_format = &surface_info->surface->format;
		u32 resize_factor = _mali_base_get_setting(MALI_SETTING_BUFFER_CAPTURE_RESIZE_FACTOR);
		u8 *source_bytes;
		external_image_format output_format;

		/* Avoid div by zero */
		if (0 == resize_factor)
		{
			resize_factor = 1;
		}

		/* Prepare the external struct to hold the image format. */
		output_format.width = (input_format->width + resize_factor-1) / resize_factor;
		output_format.height = (input_format->height + resize_factor-1) / resize_factor;
		output_format.bpp = _mali_surface_specifier_bpp(input_format);

		/* If the pixel_format is not supported, return without dumping anything. */
		if (MALI_FALSE == _mali_pixel_format_get_channel_mask(input_format, &output_format.mask_r, &output_format.mask_g, &output_format.mask_b, &output_format.mask_a))
		{
			return;
		}

		source_bytes = _mali_mem_ptr_map_area( surface_info->mem_ref->mali_memory, surface_info->mem_offset, surface_info->surface->datasize, MALI200_SURFACE_ALIGNMENT, MALI_MEM_PTR_READABLE);

		if (NULL == source_bytes)
		{
			return;
		}

		annotate_framebuffer(source_bytes, input_format, &output_format, resize_factor);

		_mali_mem_ptr_unmap_area(surface_info->mem_ref->mali_memory);
	}
}

/*
 * Callback on end of frame which sends frame content to gator.
 */
MALI_EXPORT void _mali_fbdump_dump_callback(mali_surface_info_type* surface_info)
{
	MALI_DEBUG_ASSERT_POINTER(surface_info);

	/* Annotate the framebuffer to Streamline. */
	write_framebuffer_to_streamline(surface_info);

	/* Release the buffer and the surface info structure */
	_mali_shared_mem_ref_usage_deref(surface_info->mem_ref);
	_mali_surface_deref(surface_info->surface);
	_mali_sys_free(surface_info);
}

MALI_EXPORT mali_bool _mali_fbdump_is_requested(mali_frame_builder* fbuilder)
{
	/*
	 * Keep the number of the frame that is being built in order to
	 * dump the frame at a particular rate.
	 */
	static u32 frame_number = 0;
	u32 fbdump_rate;

#if MALI_TIMELINE_PROFILING_ENABLED
	/*
	 * Top 8 bits of mali_frame_builder identifier represent the type of the EGL surface.
	 * Currently only available if the TIMELINE_PROFILING is enabled, otherwise we do not check.
	 */
	const u8 type = (fbuilder->identifier >> 24) & 0xFF;

#if defined(HAVE_ANDROID_OS)
	if (MALI_FRAME_BUILDER_TYPE_EGL_COMPOSITOR != type)
	{
		return MALI_FALSE;
	}
#else
	if (MALI_FRAME_BUILDER_TYPE_EGL_WINDOW != type)
	{
		return MALI_FALSE;
	}
#endif

#else
#error "MALI_TIMELINE_PROFILING_ENABLED must be enabled to use MALI_FRAMEBUFFER_DUMP_ENABLED."
#endif /* MALI_TIMELINE_PROFILING_ENABLED */

	/*
	 * This variable controls how often framebuffer captures are written out to Streamline.
	 * If the value is 0 then framebuffer dumping is disabled.
	 */
	fbdump_rate = _mali_base_get_setting(MALI_SETTING_BUFFER_CAPTURE_N_FRAMES);

	/* We count frames until the requested rate is reached (rate=0 doesn't make sense). */
	if (0 == fbdump_rate || (frame_number < fbdump_rate))
	{
		frame_number++;
		return MALI_FALSE;
	}

	/* Start counting for the next frame. */
	frame_number = 0;
	return MALI_TRUE;
}

MALI_EXPORT mali_err_code _mali_fbdump_setup_callbacks( struct mali_frame_builder* frame_builder)
{
	u32 i; 

	MALI_DEBUG_ASSERT_POINTER(frame_builder);

	/* early out if no colorbuffer capture is enabled or dumping not requested */	
	if (MALI_FALSE == _mali_base_get_setting(MALI_SETTING_COLORBUFFER_CAPTURE_ENABLED)) return MALI_ERR_NO_ERROR;
	if (MALI_FALSE == _mali_fbdump_is_requested(frame_builder)) return MALI_ERR_NO_ERROR;

	for ( i = 0; i < MALI200_WRITEBACK_UNIT_COUNT; i++)
	{
		u32 usage;
		mali_err_code err;
		mali_surface* surface; 

		surface = _mali_frame_builder_get_output(frame_builder, i, &usage); 
		if (NULL == surface) continue;

		/*
		 * Check:
		 *  - that we are going to dump only the color buffer,
		 *  - that the output is linear (by checking the pixel_layout),
		 */
		if (
			(MALI_OUTPUT_COLOR & usage)
			&& (MALI_PIXEL_LAYOUT_LINEAR == surface->format.pixel_layout)
		   )
		{
			/*
			 * Dump the frame buffer.
			 */

			mali_surface_info_type *surface_info = (mali_surface_info_type*)_mali_sys_calloc(1, sizeof(mali_surface_info_type));
			if (NULL != surface_info)
			{
				/* Remember the buffer ptrs, they may change. */
				surface_info->surface = surface;
				surface_info->mem_ref = surface->mem_ref;
				surface_info->mem_offset = surface->mem_offset;

				/* Set the callback to dump the frame after it has been built. */
				err = _mali_frame_builder_add_callback( frame_builder,
						(mali_frame_cb_func)_mali_fbdump_dump_callback,
						(mali_frame_cb_param) surface_info);

				/* If adding the callback failed, delete the references here. */
				if (MALI_ERR_NO_ERROR != err)
				{
					_mali_sys_free(surface_info);	/* Release the surface */
				}
				else
				{
					/*
					 * No errors: register this callback's interest in the surface and the buffer.
					 * deref calls are in _mali_surface_dump_framebuffer, after the callback is executed.
					 */
					_mali_surface_addref( surface );
					_mali_shared_mem_ref_usage_addref( surface->mem_ref );
					return MALI_ERR_NO_ERROR; /* only set up one callback per frame */
				} /* if callback setup succeeded */
			} /* if mem alloc succeeded */
		} /* if surface linear and color */
	} /* for each writeback unit */
	return MALI_ERR_NO_ERROR;
}

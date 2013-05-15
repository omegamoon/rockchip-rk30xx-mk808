# This confidential and proprietary software may be used only as
# authorised by a licensing agreement from ARM Limited
# (C) COPYRIGHT 2010-2011 ARM Limited
# ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorised
# copies and copies may only be made to the extent permitted
# by a licensing agreement from ARM Limited.

MALI_DDK="/work/trunk"

mali_drv_la_LTLIBRARIES = mali_drv.la
mali_drv_la_LDFLAGS = -module -avoid-version -L$(MALI_DDK)/lib -lMali -lUMP -lpthread
mali_drv_ladir = @moduledir@/drivers

AM_CFLAGS = @XORG_CFLAGS@ \
	-I/usr/include/libdrm \
	-I$(MALI_DDK)/include \
	-I$(MALI_DDK)/internal/include/khronos \
	-I$(MALI_DDK)/src/ump/include \
	-I$(MALI_DDK)/src/devicedrv

mali_drv_la_SOURCES = \
	mali_dri.c \
	mali_exa.c \
	mali_fbdev.c \
	mali_lcd.c

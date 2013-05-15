#
# This confidential and proprietary software may be used only as
# authorised by a licensing agreement from ARM Limited
# (C) COPYRIGHT 2011-2012 ARM Limited
# ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorised
# copies and copies may only be made to the extent permitted
# by a licensing agreement from ARM Limited.
#

ifeq ($(UMP_NO_UMP),1)

UMP_SRCS = \
	$(UMP_DIR)/arch_999_no_ump/ump_frontend.c \
	$(UMP_DIR)/arch_999_no_ump/ump_ref_drv.c

else

UMP_SRCS = \
	$(UMP_DIR)/arch_011_udd/ump_frontend.c \
	$(UMP_DIR)/arch_011_udd/ump_ref_drv.c \
	$(UMP_DIR)/arch_011_udd/ump_arch.c \
	$(UMP_DIR)/os/$(UDD_OS)/ump_uku.c \
	$(UMP_DIR)/os/$(UDD_OS)/ump_osu_memory.c \
	$(UMP_DIR)/os/$(UDD_OS)/ump_osu_locks.c

endif


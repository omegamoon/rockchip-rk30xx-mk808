#
# This confidential and proprietary software may be used only as
# authorised by a licensing agreement from ARM Limited
# (C) COPYRIGHT 2007-2012 ARM Limited
# ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorised
# copies and copies may only be made to the extent permitted
# by a licensing agreement from ARM Limited.
#

USE_UMPV2=0

# The Makefile sets up "arch" based on the CONFIG, creates the version info
# string and the __malidrv_build_info.c file, and then call the Linux build
# system to actually build the driver. After that point the Kbuild file takes
# over.

# set up defaults if not defined by the user
ARCH ?= arm

OSKOS=linux
FILES_PREFIX=

# This conditional makefile exports the global definition ARM_INTERNAL_BUILD. Customer releases will not include arm_internal.mak
-include ../../../arm_internal.mak

# Check that required parameters are supplied.
ifeq ($(CONFIG),)
$(error "CONFIG must be specified.")
endif
ifeq ($(CPU)$(KDIR),)
$(error "KDIR or CPU must be specified.")
endif

ifeq ($(USING_UMP),1)
ifeq ($(USE_UMPV2),1)
UMP_SYMVERS_FILE ?= ../umpv2/Module.symvers
else
UMP_SYMVERS_FILE ?= ../ump/Module.symvers
endif
KBUILD_EXTRA_SYMBOLS = $(realpath $(UMP_SYMVERS_FILE))
$(warning $(KBUILD_EXTRA_SYMBOLS))
endif

# Get any user defined KDIR-<names> or maybe even a hardcoded KDIR
-include KDIR_CONFIGURATION

# Define host system directory
KDIR-$(shell uname -m):=/lib/modules/$(shell uname -r)/build

ifeq ($(ARCH), arm)
	# when compiling for ARM we're cross compiling
	export CROSS_COMPILE ?= arm-none-linux-gnueabi-
endif

# look up KDIR based om CPU selection
KDIR ?= $(KDIR-$(CPU))

# validate lookup result
ifeq ($(KDIR),)
$(error No KDIR found for platform $(CPU))
endif

# report detected/selected settings
ifdef ARM_INTERNAL_BUILD
$(warning Config $(CONFIG))
$(warning Host CPU $(CPU))
$(warning OS_MEMORY $(USING_OS_MEMORY))
endif

all: $(UMP_SYMVERS_FILE)
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) modules
	@rm $(FILES_PREFIX)__malidrv_build_info.c $(FILES_PREFIX)__malidrv_build_info.o

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) clean

kernelrelease:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) kernelrelease

export CONFIG KBUILD_EXTRA_SYMBOLS

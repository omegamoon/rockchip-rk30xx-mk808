#
# This confidential and proprietary software may be used only as
# authorised by a licensing agreement from ARM Limited
# (C) COPYRIGHT 2008-2012 ARM Limited
# ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorised
# copies and copies may only be made to the extent permitted
# by a licensing agreement from ARM Limited.
#

UMP_DIR ?= .
UMP_LIB ?= libUMP
UDD_OS ?= linux
CROSS_COMPILE ?= arm-none-linux-gnueabi-
TARGET_CC ?= $(CROSS_COMPILE)gcc
TARGET_AR ?= $(CROSS_COMPILE)ar
CFLAGS += -I$(UMP_DIR)/include -I$(UMP_DIR)/include/ump -Wall -march=armv6 -mthumb-interwork -fno-strict-aliasing -Wno-strict-aliasing -Wno-long-long -O3

include ump.mak

%.o: %.c
	$(TARGET_CC) -c -o $@ $< $(CFLAGS)

UMP_OBJS := $(UMP_SRCS:.c=.o)

libUMP.so: $(UMP_OBJS)
	$(TARGET_CC) -shared -o $@ $(UMP_OBJS) $(CFLAGS)
libUMP.a: $(UMP_OBJS)
	$(TARGET_AR) rcs $@ $(UMP_OBJS)

.DEFAULT_GOAL = all
all: libUMP.so libUMP.a

clean:
	-rm -f $(UMP_OBJS) libUMP.so libUMP.a

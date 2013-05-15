/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2011-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */
Building the UMP user space library for Linux
---------------------------------------------

A simple Makefile is provided, and the UMP user space library can be built
simply by issuing make. This Makefile is setup to use the ARM GCC compiler
from CodeSourcery, and it builds for ARMv6. Modification to this Makefile
is needed in order to build for other configurations.

In order to use this library from the Mali GPU driver, invoke the Mali GPU
driver build system with the following two make variables set;
- UMP_INCLUDE_DIR should point to the include folder inside this package
- UMP_LIB should point to the built library (libUMP.so)

This does not apply to Android builds, where the Android.mk file for the
Mali GPU driver needs to be manually edited in order to add the correct
include path and link against the correct library.

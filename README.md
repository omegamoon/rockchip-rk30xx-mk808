Linux kernel release 3.x for Rockchip RK3066 - www.omegamoon.com
==============

**Build instructions:**
- Run "make mrproper" to make sure you have no stale .o files and dependencies lying around
- Run "./build_mk808_omegamoon" to compile the kernel for the MK808 using the prebuild toolchain
  
**Flash instructions for the MK808 device using Linux (use at own risk!):**
- Connect one end of the USB cable to your PC
- Press the reset button using a paperclip, and while pressed, connect the USB cable to the OTG USB port
- Release the reset button
- Run "./flash_mk808_omegamoon" to flash the kernel to the MK808
- When ready, the MK808 will be rebooted automatically

**Revision history:**
- Based on bqEdison Linux kernel version 3.0.8
- Merged with Rikomagic GPL code
- Added vpu_service code
- Added android-ndk-r8c 4.6 toolchain
- Added mk808 build script
- Added linux flash tools + flash script for mk808 (use at own risk!)
- Updated README and added build- and flash instructions

- Merged with EasyPad 971 kernel snapshot (i97-8326-1.5)

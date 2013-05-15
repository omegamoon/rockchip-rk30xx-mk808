Linux kernel release 3.x for Rockchip rk3x - www.omegamoon.com
--------------

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
- Based on bqCurie Linux kernel version 3.0.36+
- Added android-ndk-r8c 4.6 toolchain
- Added build scripts
- Added linux flash tools + flash script for mk808 (use at own risk!)

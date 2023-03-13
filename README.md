# RTIS-Bao-Hypervisor-RXM

https://github.com/bao-project/bao-demos.git guide has been followed to setup Bao on zcu104 platform. 

stress-ng package has been included in Linux guest from Buildroot configuration.

### configs
In _configs_ folder there are the config files used to configure the VMs running on top of Bao, for Xilinx ZCU104 platform, with coloring disabled, enabled for the guests, or enabled for the guests and the hypervisor.

The correct config must be set in bao-demos/wrkdir/imgs/zcu104/linux+freertos/config/linux+freertos.c file, then Bao must be re-built.

### temporal_isolation
In _temporal\_isolation_ folder there are the freeRTOS programs used for temporal isolation tests: matrix product and quicksort.

In order to correctly run FreeRTOS code, bao-demos/wrkdir/srcs/freertos/src/arch/armv8/inc/FreeRTOSConfig.h must be modified to set configTOTAL_HEAP_SIZE and configTICK_RATE_HZ depending on the desired heap size and tick rate.

FreeRTOS main.c program in bao-demos/wrkdir/srcs/freertos must be modified to run matrix product or quicksort, then FreeRTOS must be compiled, and Bao must be re-built. 

### fault_isolation
In _fault\_isolation_ folder there are the FreeRTOS and Linux programs used for fault isolation tests.

Linux main.c files must be cross-compiled through AArch64 GNU/Linux target (aarch64-none-linux-gnu) cross-compiler and ELF files must be copied to the guests via _scp_. 

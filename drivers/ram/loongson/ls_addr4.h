#ifndef __LOONGSON_DDR_H__
#define __LOONGSON_DDR_H__
#include <dm/device.h>
//#include <asm-generic/types.h>
#include <config.h>
#include <ram.h>
#include "mem_ctrl.h"

// #pragma   pack(4) 
struct mc_setting {
	struct udevice	*dev;
	struct ram_info	info;
    struct ddr_ctrl *mm_ctrl_info;
};

#endif

/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * DEV_LS2K configuration
 *
 * Copyright (c) 2022 Loongson Technologies
 * Author: Loongson
 */

#ifndef __LOONGSON_LA_COMMON_H__
#define __LOONGSON_LA_COMMON_H__

#include <linux/sizes.h>
#include "loongson_common.h"

/* Loongson LS2K0300 clock configuration. */
//#define PLL_100M
#define PLL_120M
#if defined(PLL_100M)
#define REF_FREQ				100		//参考时钟固定为100MHz
#elif defined(PLL_120M)
#define REF_FREQ				120		//参考时钟固定为120MHz
#endif

#define CORE_FREQ				CONFIG_CPU_FREQ	//现在CPU的时钟在 make menuconfig 里面选择
#define DDR_FREQ				800		//MEM 400~1200Mhz
#define APB_FREQ				100		//SB 100~200MHz, for BOOT, USB, APB, SDIO
#define NET_FREQ				200		//NETWORK 200~400MHz, for NETWORK, DC

/* Memory configuration */
#define CONFIG_SYS_BOOTPARAMS_LEN	SZ_64K
#define CONFIG_SYS_SDRAM_BASE		(0x9000000000000000) /* cached address, use the low 256MB memory */
#define CONFIG_SYS_SDRAM_SIZE		(SZ_256M)
#define CONFIG_SYS_MONITOR_BASE		CONFIG_TEXT_BASE

#ifdef CONFIG_SPL_BUILD
#define CONFIG_SPL_MAX_SIZE			SZ_256K
#define CONFIG_SPL_STACK			0x9000000090040000
#endif

/* UART configuration */
#define CONSOLE_BASE_ADDR			LS_UART0_REG_BASE
/* NS16550-ish UARTs */
#define CONFIG_SYS_NS16550_CLK		(APB_FREQ * 1000000)	// CLK_in: 100MHz
#define CONFIG_SYS_SDIO_CLK			(APB_FREQ * 1000000)	// CLK_in: 100MHz
#define CONFIG_SYS_DC_CLK			(REF_FREQ * 1000000)	// CLK_in: 100MHz

#define CONFIG_SYS_CBSIZE	4096		/* Console I/O buffer size */
#define CONFIG_SYS_MAXARGS	32		/* Max number of command args */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE 	/* Boot argument buffer size */


/* Miscellaneous configuration options */
#define CONFIG_SYS_BOOTM_LEN		(128 << 20)

/* Environment settings */
// #define CONFIG_ENV_SIZE			0x4000	/* 16KB */
#ifdef CONFIG_ENV_IS_IN_SPI_FLASH
// #define CONFIG_ENV_SIZE                 0x4000  /* 16KB */

/*
 * Environment is right behind U-Boot in flash. Make sure U-Boot
 * doesn't grow into the environment area.
 */
//#define CONFIG_BOARD_SIZE_LIMIT         CONFIG_ENV_OFFSET
#define CONFIG_BOARD_SIZE_LIMIT         0x180000
#endif

/* GMAC configuration */
#define CONFIG_DW_ALTDESCRIPTOR		// for designware ethernet driver.

/* OHCI configuration */
#ifdef CONFIG_USB_OHCI_HCD
#define CONFIG_USB_OHCI_NEW
#define CONFIG_SYS_USB_OHCI_MAX_ROOT_PORTS	1
#endif

/* video configuration */
// #define DISPLAY_BANNER_ON_VIDCONSOLE
#define CFG_SYS_SDRAM_BASE 0x9000000000000000
#define DBG_ASM

#endif /* __LOONGSON_LA_COMMON_H__ */

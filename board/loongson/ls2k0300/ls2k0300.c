// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <env.h>
#include <pci.h>
#include <usb.h>
#include <scsi.h>
#include <ahci.h>
#include <led.h>
#include <asm/io.h>
#include <dm.h>
#include <mach/loongson.h>
#include <power/regulator.h>

void loop_delay(unsigned long long loops)
{
	volatile unsigned long long counts = loops;
	while (counts--);
}

/*
 * sel 0 usb, 1 otg
 */
static void ls2k0300_usb_phy_init(unsigned long long base, int sel)
{
	unsigned int val;
	int i;

	readl(base + 0x508) &= ~(1 << 3);
	readl(base + 0x11c) &= ~(1 << (8 + sel));

	if (sel) {
		readl(base + 0x508) |= (1 << 16) | (1 << 17);
	}

	readl(base + 0x508) |= (1 << 27);
	loop_delay(10000);
	readl(base + 0x508) &= ~(1 << 27);
	readl(base + 0x508) |= (1 << (30 + sel));
	loop_delay(20000);
	if (sel == 0)
		readl(base + 0x508) &= ~(1 << (28 + sel));

	loop_delay(400000);
	if (sel == 1)
		readl(base + 0x508) &= ~(1 << (28 + sel));
	readl(base + 0x508) |= (7 << 0);
	readl(base + 0x504) = (0x18) | (0x1<<25) | (0x1<<24) | (0x0<<26) | (0x1<<27);  
	readl(base + 0x504) = (0x18) | (0x1<<25) | (0x1<<24) | (0x0<<26) | (0x0<<27);  
	do {
	    val =  readl(base + 0x504);
	} while (!(val & (1 << 28)));

	readl(base + 0x500) |= 0x4;
	readl(base + 0x504) = (0x18) | (0x1<<25) | (0x1<<24) | (0x1<<26) | (0x1<<27);  //write 0x4 in phy-addr
	readl(base + 0x504) = (0x18) | (0x1<<25) | (0x1<<24) | (0x1<<26) | (0x0<<27);
	do {
	    val =  readl(base + 0x504);
	} while (!(val & (1 << 28)));

	loop_delay(2000);

	readl(base + 0x508) |= (1 << 4);
	readl(base + 0x11c) |= (3 << 8);
	if (sel) {
		readl(base + 0x508) &= ~((1 << 16) | (1 << 17));
	}
}


static void dev_fixup(void)
{
	ls2k0300_usb_phy_init(PHYS_TO_UNCACHED(0x16000000), 0); 
	ls2k0300_usb_phy_init(PHYS_TO_UNCACHED(0x16000000), 1);
#if 0
	/* uart 0 and 1 be 2 line mode */
	val = readl(LS_GENERAL_CFG0);
	val |= (0xff << 20);
	writel(val, LS_GENERAL_CFG0);	

	/*RTC toytrim rtctrim must init to 0, otherwise time can not update*/
	writel(0x0, LS_TOY_TRIM_REG);
	writel(0x0, LS_RTC_TRIM_REG);
#endif
}

/*enable fpu regs*/
static void __maybe_unused fpu_enable(void)
{
	asm (
		"csrrd $r4, 0x2;\n\t"
		"ori   $r4, $r4, 1;\n\t"
		"csrwr $r4, 0x2;\n\t"
		:::"$r4"
	);
}

/*disable fpu regs*/
static void __maybe_unused fpu_disable(void)
{
	asm (
		"csrrd $r4, 0x2;\n\t"
		"ori   $r4, $r4, 1;\n\t"
		"xori  $r4, $r4, 1;\n\t"
		"csrwr $r4, 0x2;\n\t"
		:::"$r4"
	);
}


#ifdef CONFIG_BOARD_EARLY_INIT_F
int ls_board_early_init_f(void)
{
	// fpu_enable();
	dev_fixup();

	return 0;
}
#endif


static void regulator_init(void)
{
#ifdef CONFIG_DM_REGULATOR
	regulators_enable_boot_on(false);
#endif
}

#ifdef CONFIG_BOARD_EARLY_INIT_R
int ls_board_early_init_r(void)
{
	regulator_init();

	return 0;
}
#endif

#ifdef CONFIG_BOARD_LATE_INIT
int ls_board_late_init(void)
{

	return 0;
}
#endif


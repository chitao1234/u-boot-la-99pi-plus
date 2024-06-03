// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2015 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 */

#include <common.h>
#include <asm/io.h>
#include <dm.h>
#include <rtc.h>

/**
 * Loongson-2K rtc register
 */

#define TOY_TRIM_REG   0x20
#define TOY_WRITE0_REG 0x24
#define TOY_WRITE1_REG 0x28
#define TOY_READ0_REG  0x2c
#define TOY_READ1_REG  0x30
#define TOY_MATCH0_REG 0x34
#define TOY_MATCH1_REG 0x38
#define TOY_MATCH2_REG 0x3c
#define RTC_CTRL_REG   0x40
#define RTC_TRIM_REG   0x60
#define RTC_WRITE0_REG 0x64
#define RTC_READE0_REG 0x68
#define RTC_MATCH0_REG 0x6c
#define RTC_MATCH1_REG 0x70
#define RTC_MATCH2_REG 0x74

/**
 * shift bits and filed mask
 */
#define TOY_MON_MASK   0x3f
#define TOY_DAY_MASK   0x1f
#define TOY_HOUR_MASK  0x1f
#define TOY_MIN_MASK   0x3f
#define TOY_SEC_MASK   0x3f
#define TOY_MSEC_MASK  0xf

#define TOY_MON_SHIFT  26
#define TOY_DAY_SHIFT  21
#define TOY_HOUR_SHIFT 16
#define TOY_MIN_SHIFT  10
#define TOY_SEC_SHIFT  4
#define TOY_MSEC_SHIFT 0

/* shift bits for TOY_MATCH */
#define TOY_MATCH_YEAR_SHIFT 26
#define TOY_MATCH_MON_SHIFT  22
#define TOY_MATCH_DAY_SHIFT  17
#define TOY_MATCH_HOUR_SHIFT 12
#define TOY_MATCH_MIN_SHIFT  6
#define TOY_MATCH_SEC_SHIFT  0

/* Filed mask bits for TOY_MATCH */
#define TOY_MATCH_YEAR_MASK  0x3f
#define TOY_MATCH_MON_MASK   0xf
#define TOY_MATCH_DAY_MASK   0x1f
#define TOY_MATCH_HOUR_MASK  0x1f
#define TOY_MATCH_MIN_MASK   0x3f
#define TOY_MATCH_SEC_MASK   0x3f

/* TOY Enable bits */
#define RTC_ENABLE_BIT		(1UL << 13)
#define TOY_ENABLE_BIT		(1UL << 11)
#define OSC_ENABLE_BIT		(1UL << 8)

/* interface for rtc read and write */
#define rtc_write(val, addr)   writel(val, rtc_reg_base + (addr))
#define rtc_read(addr)         readl(rtc_reg_base + (addr))

static void __iomem *rtc_reg_base;

static int ls_rtc_get(struct udevice *dev, struct rtc_time *time)
{
	unsigned int val;

	val = rtc_read(TOY_READ1_REG);
	time->tm_year = val;
	val = rtc_read(TOY_READ0_REG);
	time->tm_sec = (val >> TOY_SEC_SHIFT) & TOY_SEC_MASK;
	time->tm_min = (val >> TOY_MIN_SHIFT) & TOY_MIN_MASK;
	time->tm_hour = (val >> TOY_HOUR_SHIFT) & TOY_HOUR_MASK;
	time->tm_mday = (val >> TOY_DAY_SHIFT) & TOY_DAY_MASK;
	time->tm_mon = ((val >> TOY_MON_SHIFT) & TOY_MON_MASK) - 1;
	rtc_calc_weekday(time);

	return 0;
}

static int ls_rtc_set(struct udevice *dev, const struct rtc_time *time)
{
	unsigned int val = 0;

	val |= (time->tm_sec << TOY_SEC_SHIFT);
	val |= (time->tm_min << TOY_MIN_SHIFT);
	val |= (time->tm_hour << TOY_HOUR_SHIFT);
	val |= (time->tm_mday << TOY_DAY_SHIFT);
	val |= ((time->tm_mon + 1) << TOY_MON_SHIFT);
	rtc_write(val, TOY_WRITE0_REG);
	val = time->tm_year;
	rtc_write(val, TOY_WRITE1_REG);

	return 0;
}

static int ls_rtc_reset(struct udevice *dev)
{
	return 0;
}

static int ls_rtc_probe(struct udevice *dev)
{
	unsigned int reg_val;
	rtc_reg_base = PHYS_TO_UNCACHED((phys_addr_t)dev_read_addr(dev));

	if (rtc_reg_base == FDT_ADDR_T_NONE)
		return -EINVAL;
	/* enable rtc */
	reg_val = rtc_read(RTC_CTRL_REG);
	reg_val |= TOY_ENABLE_BIT | OSC_ENABLE_BIT;
	rtc_write(reg_val, RTC_CTRL_REG);

	return 0;
}

static const struct rtc_ops ls_rtc_ops = {
	.get = ls_rtc_get,
	.set = ls_rtc_set,
	.reset = ls_rtc_reset,
};

static const struct udevice_id ls_rtc_ids[] = {
	{ .compatible = "loongson,ls-rtc" },
	{ }
};

U_BOOT_DRIVER(ls_rtc) = {
	.name	= "ls-rtc",
	.id	= UCLASS_RTC,
	.probe	= ls_rtc_probe,
	.of_match = ls_rtc_ids,
	.ops	= &ls_rtc_ops,
};

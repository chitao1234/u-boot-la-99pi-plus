/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Loongson Soc`s display controller driver.
 *
 * (C) Copyright 2022 Jianhui Wang <wangjianhui@loongson.cn>
 */

#include <common.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/fb.h>
#include <linux/compat.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <malloc.h>
#include <video_fb.h>
#include <panel.h>
#include <dm.h>
#include <dm/read.h>
#include <video.h>
#include <display.h>
#include <video_bridge.h>
#include <linux/delay.h>
#include <clk.h>
#include <i2c.h>
#include <edid.h>
#include <env.h>

// #define LS_DC_CURSOR
#define LS_FB_ALIGN					(0x0)

#define ls_write_reg(reg, val)		writel((val), (reg))
#define PIX_FMT_DEFAULT         	PIX_FMT_RGB565

#define LS_FB_MAX_WIDTH             (2048)
#define LS_FB_MAX_HEIGHT            (2048)
#define LS_FB_SIZE					(LS_FB_MAX_WIDTH*LS_FB_MAX_HEIGHT*4)
#define LS_DC_CURSOR_SZ				(0x1000)
#define LS_DC_MAX_DVO               (2)

#define PIX_FMT_NONE		    0
#define PIX_FMT_RGB444		    1
#define PIX_FMT_RGB555		    2
#define PIX_FMT_RGB565		    3
#define PIX_FMT_RGB888	        4

#define DDC_SLAVE_ADDR				(0x50)

#define DVO_REG_BASE_OFFSET			(0x10)
// DC registers
#define LS_DC_FB_CONF				(0x1240)
#define LS_DC_FB_ADDR0				(0x1260)
#define LS_DC_FB_ADDR0_HI			(0x15a0)
#define LS_DC_FB_STRIDE				(0x1280)
#define LS_DC_FB_ORIG				(0x1300)
#define LS_DC_DITHER_CONF			(0x1360)
#define LS_DC_DITHER_TABLE_LOW		(0x1380)
#define LS_DC_DITHER_TABLE_HIGH		(0x13a0)
#define LS_DC_PAN_CONF				(0x13c0)
#define LS_DC_PAN_TIMING			(0x13e0)
#define LS_DC_HDISPLAY				(0x1400)
#define LS_DC_HSYNC					(0x1420)
#define LS_DC_VDISPLAY				(0x1480)
#define LS_DC_VSYNC					(0x14a0)
#define LS_DC_FB_ADDR1				(0x1580)
#define LS_DC_FB_ADDR1_HI			(0x15c0)
#define LS_DC_CURSOR_CONF			(0x1520)
#define LS_DC_CURSOR_ADDR			(0x1530)
#define LS_DC_CURSOR_LOC_ADDR		(0x1540)
#define LS_DC_CURSOR_BACK			(0x1550)
#define LS_DC_CURSOR_FORE			(0x1560)

#define LS_FB_CONF_RESET			(1 << 20)
#define LS_FB_CONF_OUTPUT_EN		(1 << 8)
#define LS_FB_CONF_FB_NUM_SHIFT		(11)
#define LS_FB_CONF_FB_NUM_MASK		(0x1)

#define LS_PANEL_CONF_DE			(1 << 0)

enum ls_connector_type {
	LS_CONNECTOR_UNKNOWN = 0,
	LS_CONNECTOR_PANEL,
	LS_CONNECTOR_VGA,
	LS_CONNECTOR_HDMI,
	LS_CONNECTOR_BRIDGE,
};

struct ls_pll_cfg {
	unsigned int l2_div;
	unsigned int l1_loopc;
	unsigned int l1_frefc;
};

struct ls_mode_setting {
	int pclk_khz;
	int hactive, hsync_start, hsync_end, htotal;
	int vactive, vsync_start, vsync_end, vtotal;
};

struct ls_video_priv {
	void __iomem        	*reg_base;
	void __iomem        	*pix_pll_base;
	void __iomem        	*fb_base;
#ifdef LS_DC_CURSOR
	void __iomem        	*cursor_addr;
#endif
	struct udevice      	*ddc_bus;
	struct udevice      	*connector;
	enum ls_connector_type  conn_type;
	struct display_timing   timing;
	struct ls_pll_cfg   	pix_pll;
	unsigned int            id;
	unsigned int            pix_fmt;
	long long               refclk;
};

static struct display_timing vga_timing_default = {
	.hactive = {800, 800, 800},
	.vactive = {600, 600, 600},
	.hdmi_monitor = false,
};
static struct display_timing timing_default = {
	.pixelclock = {65000000, 65000000, 65000000},

	.hactive = {1024, 1024, 1024},
	.hfront_porch = {24, 24, 24},
	.hback_porch = {160, 160, 160},
	.hsync_len = {136, 136, 136},

	.vactive = {768, 768, 768},
	.vfront_porch = {3, 3, 3},
	.vback_porch = {29, 29, 29},
	.vsync_len = {6, 6, 6},

	.flags = DISPLAY_FLAGS_HSYNC_LOW | DISPLAY_FLAGS_VSYNC_LOW
				| DISPLAY_FLAGS_DE_HIGH | DISPLAY_FLAGS_PIXDATA_NEGEDGE,
	.hdmi_monitor = false,
};

/*gtf 800 480 60*/
static struct ls_mode_setting vgamode[] = {
	{	28000,	480,	510,	518,	558,	800,	814,	824,	836,	},	/*"480x800_60.00" */
	//{	28560,	640,	664,	728,	816,	480,	481,	484,	500,	},	/*"640x480_70.00" */
	{	23860,	640,	656,	720,	800,	480,	481,	484,	497,	},	/*"640x480_60.00" */
	{	33100,	640,	672,	736,	832,	640,	641,	644,	663,	},	/*"640x640_60.00" */
	{	39690,	640,	672,	736,	832,	768,	769,	772,	795,	},	/*"640x768_60.00" */
	{	42130,	640,	680,	744,	848,	800,	801,	804,	828,	},	/*"640x800_60.00" */
	{	35840,	800,	832,	912,	1024,	480,	481,	484,	500,	},	/*"800x480_70.00" */
	{       38220,  800,    832,    912,    1024,   600,    601,    604,    622,    },      /*"800x600_60.00" */
	{	40730,	800,	832,	912,	1024,	640,	641,	644,	663,	},	/*"800x640_60.00" */
	{	40010,	832,	864,	952,	1072,	600,	601,	604,	622,	},	/*"832x600_60.00" */
	{	40520,	832,	864,	952,	1072,	608,	609,	612,	630,	},	/*"832x608_60.00" */
	{	45980,	960,	864,	952,	1072,	600,	609,	612,	630,	},	/*"960x600_60.00" */
	{	38170,	1024,	1048,	1152,	1280,	480,	481,	484,	497,	},	/*"1024x480_60.00" */
	{	48960,	1024,	1064,	1168,	1312,	600,	601,	604,	622,	},	/*"1024x600_60.00" */
	{	52830,	1024,	1072,	1176,	1328,	640,	641,	644,	663,	},	/*"1024x640_60.00" */
	{	64110,	1024,	1080,	1184,	1344,	768,	769,	772,	795,	},	/*"1024x768_60.00" */
	{	71380,	1152,	1208,	1328,	1504,	764,	765,	768,	791,	},	/*"1152x764_60.00" */
	{	83460,	1280,	1344,	1480,	1680,	800,	801,	804,	828,	},	/*"1280x800_60.00" */
	{	108880,	1280,	1360,	1496,	1712,	1024,	1025,	1028,	1060,	},	/*"1280x1024_60.00" */
	{	85860,	1368,	1440,	1584,	1800,	768,	769,	772,	795,	},	/*"1368x768_60.00" */
	{	93800,	1440,	1512,	1664,	1888,	800,	801,	804,	828,	},	/*"1440x800_60.00" */
	{	120280,	1440,	1528,	1680,	1920,	900,	901,	904,	935,	},	/*"1440x900_67.00" */
	{	172800,	1920,	2040,	2248,	2576,	1080,	1081,	1084,	1118,	},	/*"1920x1080_60.00" */
};

static int ls_disptiming_to_modesetting(struct ls_video_priv *priv,
					struct display_timing *timing, struct ls_mode_setting *setting)
{
	int i, mode_id = 0;
	if (!priv || !timing || !setting)
		return -EINVAL;

	if (priv->conn_type == LS_CONNECTOR_VGA) {
		for (i = 0; i < ARRAY_SIZE(vgamode); i++) {
			if (vgamode[i].hactive == timing->hactive.typ 
					&& vgamode[i].vactive == timing->vactive.typ) {
				mode_id = i;
				break;
			}
		}

		debug("mode index: %d\n", mode_id);
		memcpy(setting, &vgamode[mode_id], sizeof(struct ls_mode_setting));
	} else {
		setting->pclk_khz = timing->pixelclock.typ / 1000;
		setting->hactive = timing->hactive.typ;
		setting->hsync_start = timing->hactive.typ + timing->hfront_porch.typ;
		setting->hsync_end = setting->hsync_start + timing->hsync_len.typ;
		setting->htotal = setting->hsync_end + timing->hback_porch.typ;
		setting->vactive = timing->vactive.typ;
		setting->vsync_start = timing->vactive.typ + timing->vfront_porch.typ;
		setting->vsync_end = setting->vsync_start + timing->vsync_len.typ;
		setting->vtotal = setting->vsync_end + timing->vback_porch.typ;
	}

	return 0;
}

static void config_pll(void __iomem *pll_base, struct ls_pll_cfg *pix_pll)
{
	unsigned int out;

	out = (pix_pll->l2_div << 24) | ((pix_pll->l1_loopc) << 15) |
				((pix_pll->l1_frefc) << 8);

	writel(0, pll_base);
	writel(1 << 5, pll_base);	//power down pll first
	writel(out, pll_base);
	out = (out | (1 << 3));
	writel(out, pll_base);

	debug("pll out[0x%x]\n", out);

	// wait pll locked
	while (!(readl(pll_base) & 0x80)) ;

	writel((out | 1), pll_base);
}

static int cal_freq(unsigned int pixclock_khz, struct ls_pll_cfg * pll_config)
{
	unsigned int pstdiv, loopc, frefc;
	unsigned long a, b, c;
	unsigned long min = 1000;

	for (pstdiv = 1; pstdiv < 128; pstdiv++) {
		a = (unsigned long)pixclock_khz * pstdiv;
		for (frefc = 4; frefc < 6; frefc++) {
			for (loopc = 24; loopc < 161; loopc++) {
				if ((loopc < 12 * frefc) ||
						(loopc > 32 * frefc))
					continue;

				b = 120000L * loopc / frefc;
				c = (a > b) ? (a - b) : (b - a);
				if (c < min) {
					pll_config->l2_div = pstdiv;
					pll_config->l1_loopc = loopc;
					pll_config->l1_frefc = frefc;

					debug("pll found pstdiv[0x%x], loopc[0x%x], frefc[0x%x]  for pixclk[%u khz]\n",
								pstdiv, loopc, frefc, pixclock_khz);
					return 0;
				}
			}
		}
	}

	debug("calculate PIX freq error!!!\n");
	return -EINVAL;
}

#ifdef LS_DC_CURSOR
static void config_cursor(struct ls_video_priv *priv)
{
	void __iomem *base = priv->reg_base;
	phys_addr_t cursor_addr = virt_to_phys(priv->cursor_addr) & 0xffffffff;

	/* framebuffer Cursor Configuration */
	ls_write_reg(base + LS_DC_CURSOR_CONF, 0x00020200);
	/* framebuffer Cursor Address */
	ls_write_reg(base + LS_DC_CURSOR_ADDR, cursor_addr);
	/* framebuffer Cursor Location */
	ls_write_reg(base + LS_DC_CURSOR_LOC_ADDR, 0x00060122);
	/* framebuffer Cursor Background */
	ls_write_reg(base + LS_DC_CURSOR_BACK, 0x00eeeeee);
	ls_write_reg(base + LS_DC_CURSOR_FORE, 0x00aaaaaa);
}
#endif

static int config_fb(struct ls_video_priv *priv, int dvo_id)
{
	phys_addr_t base = (phys_addr_t)priv->reg_base +
							dvo_id * DVO_REG_BASE_OFFSET;
	phys_addr_t fb_addr = virt_to_phys(priv->fb_base) & 0xffffffff;
	struct ls_mode_setting mode;
	int ret;
	// u32 val;

	ret = ls_disptiming_to_modesetting(priv, &priv->timing, &mode);
	if (ret < 0) {
		debug("get mode setting failed (err: %d)\n", ret);
		return ret;
	}

	/* Disable the panel output*/
	// ls_write_reg(base + LS_DC_PAN_CONF, 0x0);
	ls_write_reg(base + LS_DC_FB_CONF, 0x00000000);
	ls_write_reg(base + LS_DC_FB_ADDR0_HI, 0);
	ls_write_reg(base + LS_DC_FB_ADDR1_HI, 0);
	ls_write_reg(base + LS_DC_FB_ADDR0, fb_addr);
	ls_write_reg(base + LS_DC_FB_ADDR1, fb_addr);
	ls_write_reg(base + LS_DC_DITHER_CONF, 0x00000000);
	ls_write_reg(base + LS_DC_DITHER_TABLE_LOW, 0x00000000);
	ls_write_reg(base + LS_DC_DITHER_TABLE_HIGH, 0x00000000);

	ls_write_reg(base + LS_DC_PAN_CONF, 0x0);
	if (priv->conn_type == LS_CONNECTOR_VGA)
		ls_write_reg(base + LS_DC_PAN_CONF, 0x80001311);
	else
		ls_write_reg(base + LS_DC_PAN_CONF, 0x80001111);

	ls_write_reg(base + LS_DC_PAN_TIMING, 0x00000000);
	ls_write_reg(base + LS_DC_FB_ORIG, 0x0);

	ls_write_reg(base + LS_DC_HDISPLAY,
		(mode.htotal << 16) | mode.hactive);
	ls_write_reg(base + LS_DC_HSYNC,
		0x40000000 | (mode.hsync_end << 16) | mode.hsync_start);
	ls_write_reg(base + LS_DC_VDISPLAY,
		(mode.vtotal << 16) | mode.vactive);
	ls_write_reg(base + LS_DC_VSYNC,
		0x40000000 | (mode.vsync_end << 16) | mode.vsync_start);

	switch (priv->pix_fmt)
	{
	case PIX_FMT_RGB444:
		ls_write_reg(base + LS_DC_FB_CONF, LS_FB_CONF_RESET | LS_FB_CONF_OUTPUT_EN | 0x1);
		ls_write_reg(base + LS_DC_FB_STRIDE, (mode.hactive * 2 + LS_FB_ALIGN) & ~LS_FB_ALIGN);
		break;
	case PIX_FMT_RGB555:
		ls_write_reg(base + LS_DC_FB_CONF, LS_FB_CONF_RESET | LS_FB_CONF_OUTPUT_EN | 0x2);
		ls_write_reg(base + LS_DC_FB_STRIDE, (mode.hactive * 2 + LS_FB_ALIGN) & ~LS_FB_ALIGN);
		break;
	case PIX_FMT_RGB565:
		ls_write_reg(base + LS_DC_FB_CONF, LS_FB_CONF_RESET | LS_FB_CONF_OUTPUT_EN | 0x3);
		ls_write_reg(base + LS_DC_FB_STRIDE, (mode.hactive * 2 + LS_FB_ALIGN) & ~LS_FB_ALIGN);
		break;
	case PIX_FMT_RGB888:
		ls_write_reg(base + LS_DC_FB_CONF, LS_FB_CONF_RESET | LS_FB_CONF_OUTPUT_EN | 0x4);
		ls_write_reg(base + LS_DC_FB_STRIDE, (mode.hactive * 4 + LS_FB_ALIGN) & ~LS_FB_ALIGN);
		break;
	default:
		ls_write_reg(base + LS_DC_FB_CONF, LS_FB_CONF_RESET | LS_FB_CONF_OUTPUT_EN | 0x4);
		ls_write_reg(base + LS_DC_FB_STRIDE, (mode.hactive * 4 + LS_FB_ALIGN) & ~LS_FB_ALIGN);
		break;
	}

	return 0;
}

static int ls_dc_init(struct ls_video_priv *priv)
{
	struct ls_mode_setting mode;
	int ret;

	ret = ls_disptiming_to_modesetting(priv, &priv->timing, &mode);
	if (ret < 0) {
		debug("get mode setting failed (err: %d)\n", ret);
		return ret;
	}

	ret = cal_freq(mode.pclk_khz, &priv->pix_pll);
	if (!ret)
		config_pll(priv->pix_pll_base, &priv->pix_pll);
#ifdef LS_DC_CURSOR
	for (int i = 0; i < LS_DC_CURSOR_SZ; i += 4)
		*(u32 *)(priv->cursor_addr + i) = 0x88f31f4f;
	config_cursor(priv);
#endif
	config_fb(priv, priv->id);

	debug("display controller config complete!\n");
	return 0;
}

static int ls_read_edid(struct ls_video_priv *priv, int block, u8 *buf)
{
	int shift = block * EDID_SIZE;
	struct udevice *chip;
	int ret;
#ifdef CONFIG_VIDEO_BRIDGE
	if (priv->conn_type == LS_CONNECTOR_BRIDGE) {
		ret = video_bridge_read_edid(priv->connector, buf, EDID_SIZE);
		if (ret < 0)
			return ret;
	}
#endif

//#if defined(CONFIG_DM_I2C)
#if 0
	if (priv->ddc_bus) {
		ret = i2c_get_chip(priv->ddc_bus, DDC_SLAVE_ADDR, 1, &chip);
		if (ret) {
			debug("get ddc chip failed (err: %d)\n", ret);
			return ret;
		}

		return dm_i2c_read(chip, shift, buf, EDID_SIZE);
	}
#endif
	return 0;
}

extern int ls_fill_display_timing_by_name(struct display_timing* timing, char* name);

static int env_panel_get_display_timing(struct ls_video_priv *priv)
{
	int ret;
	char* value = NULL;

	value = env_get("panel");

	if (!value)
		return -EINVAL;

	// use dts desc panel
	if (!strcmp(value, "default"))
		return 1;

	ret = ls_fill_display_timing_by_name(&priv->timing, value);
	value = NULL;
	return ret;
}

static int ls_video_get_display_timing(struct udevice *dev)
{
	struct ls_video_priv *priv = dev_get_priv(dev);
	u8 edid[EDID_SIZE];
	int panel_bits_per_colour;
	int ret = -1;

	if (priv->conn_type == LS_CONNECTOR_PANEL) {
		ret = env_panel_get_display_timing(priv);
		if (ret)
			ret = dev_decode_display_timing(priv->connector, 0, &priv->timing);
	} else if (priv->conn_type == LS_CONNECTOR_VGA) {
		priv->timing = vga_timing_default;
		ret = 0;
	} else {
		memset(&edid, 0, sizeof(struct edid1_info));
		ret = ls_read_edid(priv, 0, edid);
		if (ret < 0) {
			debug("get edid failed (err: %d)\n", ret);
			goto failed;
		}

		// edid_print_info((struct edid1_info *)edid);

		// fix edid video_input_definition
		// set it to digital input for edid_get_timing can decode it.
		((struct edid1_info *)edid)->video_input_definition |= (1 << 7);

		ret = edid_get_timing(edid, EDID_SIZE, &priv->timing, &panel_bits_per_colour);
		if (ret) {
			debug("get display timing from edid failed (err: %d)\n", ret);
			goto failed;
		}
	}

failed:
	if (ret < 0) {
		debug("get display timing failed (err: %d), use the default timing.\n", ret);
		memcpy(&priv->timing, &timing_default, sizeof(struct display_timing));
	}

	// THIS cause uclass_get_device_by_phandle(UCLASS_PANEL fail
	debug("display timing:\n\tpixclk: %u\n"
			"\thdisp: %u, hfp: %u, hbp: %u, hsynclen: %u\n"
			"\tvdisp: %u, vfp: %u, vbp: %u, vsynclen: %u\n"
			"\tflag: 0x%x\n",
			priv->timing.pixelclock.typ,
			priv->timing.hactive.typ,
			priv->timing.hfront_porch.typ,
			priv->timing.hback_porch.typ,
			priv->timing.hsync_len.typ,
			priv->timing.vactive.typ,
			priv->timing.vfront_porch.typ,
			priv->timing.vback_porch.typ,
			priv->timing.vsync_len.typ,
			priv->timing.flags);

	return 0;
}

// static int ls_dc_sync(struct udevice *vid)
// {
// 	return 0;
// }

static int ls_video_parse_dt(struct udevice *dev)
{
	struct ls_video_priv *priv = dev_get_priv(dev);
	phys_addr_t reg_base, addr;
	struct clk ref_clk;
	const char *connector_type, *pixfmt;
	ofnode parent;
	u32 id = 0;
	int ret;

	ret = dev_read_u32(dev, "reg", &id);
	if (ret || id >= LS_DC_MAX_DVO) {
		debug("%s: the value of reg prop is invalid\n", dev->name);
		return -EINVAL;
	}
	priv->id = id;

	parent = ofnode_get_parent(dev->node_);
	if (!ofnode_valid(parent)) {
		debug("get parent node failed\n");
		return -ENODEV;
	}

	reg_base = ofnode_get_addr(parent);
	if (reg_base == (phys_addr_t)FDT_ADDR_T_NONE) {
		debug("get parent reg base addr failed\n");
		return -ENODEV;
	}

	priv->reg_base = ioremap(reg_base, 0);
	debug("%s reg base: %p\n", dev->name, priv->reg_base);

	// pix pll config register base.
	priv->pix_pll_base = NULL;
	addr = ofnode_get_addr_index(parent, 1 + priv->id);
	if (addr != (phys_addr_t)FDT_ADDR_T_NONE)
		priv->pix_pll_base = ioremap(addr, 0);

	// framebuffer addr
	priv->fb_base = NULL;
	addr = ofnode_get_addr_index(parent, 3);
	if (addr != (phys_addr_t)FDT_ADDR_T_NONE) {
		addr = addr + priv->id * LS_FB_SIZE;
		priv->fb_base = map_sysmem(addr, 0);
	}

	ret = clk_get_by_name(dev, "refclk", &ref_clk);
	if (!ret) {
		priv->refclk = clk_get_rate(&ref_clk);
	} else {
		debug("Cannot get refclk, use default: 100MHz\n");
		priv->refclk = dev_read_u32_default(dev, "clock-frequency",
						   CONFIG_SYS_DC_CLK);
		if (!priv->refclk)
			priv->refclk = CONFIG_SYS_DC_CLK;
	}

	connector_type = dev_read_string(dev, "connector-type");
	if (connector_type) {
		if (!strcmp(connector_type, "panel"))
			priv->conn_type = LS_CONNECTOR_PANEL;
		else if (!strcmp(connector_type, "vga"))
			priv->conn_type = LS_CONNECTOR_VGA;
		else if (!strcmp(connector_type, "hdmi"))
			priv->conn_type = LS_CONNECTOR_HDMI;
		else if (!strcmp(connector_type, "bridge"))
			priv->conn_type = LS_CONNECTOR_BRIDGE;
	}

	pixfmt = dev_read_string(dev, "pix-fmt");
	if (pixfmt) {
		if (!strcmp(pixfmt, "rgb444"))
			priv->pix_fmt = PIX_FMT_RGB444;
		if (!strcmp(pixfmt, "rgb555"))
			priv->pix_fmt = PIX_FMT_RGB555;
		if (!strcmp(pixfmt, "rgb565"))
			priv->pix_fmt = PIX_FMT_RGB565;
		if (!strcmp(pixfmt, "rgb888"))
			priv->pix_fmt = PIX_FMT_RGB888;
	}

	if (priv->conn_type == LS_CONNECTOR_HDMI) {
		ret = uclass_get_device_by_phandle(UCLASS_DISPLAY, dev, "connector",
					&priv->connector);
		if (ret) {
			debug("%s: Cannot find connector for '%s' (ret=%d)\n", __func__,
				dev->name, ret);
			priv->connector = NULL;
		}
	} else if (priv->conn_type == LS_CONNECTOR_PANEL) {
		ret = uclass_get_device_by_phandle(UCLASS_PANEL, dev, "connector",
						&priv->connector);
		if (ret) {
			debug("%s: Cannot find panel for '%s' (ret=%d)\n", __func__,
				dev->name, ret);
			return ret;
		}
#ifdef CONFIG_VIDEO_BRIDGE
	} else if (priv->conn_type == LS_CONNECTOR_BRIDGE) {
		ret = uclass_get_device_by_phandle(UCLASS_VIDEO_BRIDGE, dev, "connector",
						&priv->connector);
		if (ret) {
			debug("%s: Cannot find bridge for '%s' (ret=%d)\n", __func__,
				dev->name, ret);
			return ret;
		}
#endif
	} else {
		priv->connector = NULL;
	}

	ret = uclass_get_device_by_phandle(UCLASS_I2C, dev, "ddc-i2c-bus",
					&priv->ddc_bus);
	if (ret) {
		debug("Cannot get ddc bus\n");
		priv->ddc_bus = NULL;
	}

	return 0;
}


static int ls_video_probe(struct udevice *dev)
{
	struct video_uc_plat *uc_plat = dev_get_uclass_plat(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	struct ls_video_priv *priv = dev_get_priv(dev);
	int ret;

	debug("%s uc_plat: base 0x%lx, size 0x%x\n",
		dev->name, uc_plat->base, uc_plat->size);
	
	priv->ddc_bus = NULL;
	priv->connector = NULL;
	priv->conn_type = LS_CONNECTOR_UNKNOWN;
	priv->pix_fmt = PIX_FMT_DEFAULT;
#ifdef LS_DC_CURSOR
	priv->cursor_addr = memalign(ARCH_DMA_MINALIGN, LS_DC_CURSOR_SZ);
#endif

	ret = ls_video_parse_dt(dev);
	if (ret)
		return ret;

	if (!priv->fb_base && !uc_plat->base) {
		debug("%s: framebuffer addr err!\n", dev->name);
		return -ENOMEM;
	}

	if (priv->fb_base) {
		gd->fb_base = uc_plat->base = (ulong)priv->fb_base;
	} else {
		// use the uboot reserved video framebuffer
		priv->fb_base = map_sysmem((phys_addr_t)uc_plat->base, 0);
		gd->fb_base = uc_plat->base;
	}

	printf("frame buffer addr: 0x%p\n", priv->fb_base);
	ret = ls_video_get_display_timing(dev);
	if (ret < 0) {
		debug("get display timing err: %d\n", ret);
		return ret;
	}

	uc_priv->xsize = priv->timing.hactive.typ;
	uc_priv->ysize = priv->timing.vactive.typ;
	if (priv->pix_fmt == PIX_FMT_RGB888) {
		uc_priv->bpix = VIDEO_BPP32;
		uc_priv->format = VIDEO_X8R8G8B8;
	} else {
		uc_priv->bpix = VIDEO_BPP16;
		uc_priv->format = VIDEO_UNKNOWN;
	}

	ret = ls_dc_init(priv);
	if (ret)
		return ret;

	if (priv->connector) {
		if (priv->conn_type == LS_CONNECTOR_HDMI)
			display_enable(priv->connector, (1 << VIDEO_BPP32), NULL);
#ifdef CONFIG_VIDEO_BRIDGE
		else if (priv->conn_type == LS_CONNECTOR_BRIDGE)
			video_bridge_attach(priv->connector);
#endif
		else if (priv->conn_type == LS_CONNECTOR_PANEL)
			panel_enable_backlight(priv->connector);
	}

	debug("%s - connector type: %s, pix fmt: %d, display resolution: %d x %d\n", dev->name, 
			priv->conn_type == LS_CONNECTOR_PANEL ? "Panel" :
				priv->conn_type == LS_CONNECTOR_VGA ? "VGA" :
				priv->conn_type == LS_CONNECTOR_HDMI ? "HDMI" : 
				priv->conn_type == LS_CONNECTOR_BRIDGE ? "Bridge" :"Unknown",
			priv->pix_fmt, priv->timing.hactive.typ, priv->timing.vactive.typ);

	return 0;
}

static int ls_video_bind(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	plat->size = LS_FB_SIZE;
	return 0;
}

// static const struct video_ops ls_video_ops = {
// 	.video_sync = ls_dc_sync,
// };

static const struct udevice_id ls_video_ids[] = {
	{ .compatible = "loongson,ls-dc-dvo" },
	{ }
};

U_BOOT_DRIVER(ls_video) = {
	.name	= "ls-video",
	.id	= UCLASS_VIDEO,
	.of_match = ls_video_ids,
	// .ops = &ls_video_ops,
	.bind	= ls_video_bind,
	.probe	= ls_video_probe,
	.priv_auto = sizeof(struct ls_video_priv),
	.flags	= DM_FLAG_PRE_RELOC,
};

#include <stdlib.h>
#include "fdtdec.h"
#include "linux/errno.h"

struct display_timing_mimics_dts {
	char* name;
	char* option;
	u32 pixelclock;

	u32 hactive;		/* hor. active video */
	u32 hfront_porch;	/* hor. front porch */
	u32 hback_porch;	/* hor. back porch */
	u32 hsync_len;		/* hor. sync len */

	u32 vactive;		/* ver. active video */
	u32 vfront_porch;	/* ver. front porch */
	u32 vback_porch;	/* ver. back porch */
	u32 vsync_len;		/* ver. sync len */

	bool de_active;
	bool hsync_active;
	bool vsync_active;
	bool pixelclk_active;

	bool hdmi_monitor;	/* is hdmi monitor? */
};

static struct display_timing_mimics_dts display_timing_mimics_set[] = {
	{
		.name	= "BP101WX1-206",
		.option = "BOE BP101WX1-206 1280x800 60Hz",

		.pixelclock	= 71000000,

		.hactive	= 1280,
		.hfront_porch		= 8,
		.hback_porch		= 10,
		.hsync_len	= 32,

		.vactive	= 800,
		.vfront_porch	= 3,
		.vback_porch	= 3,
		.vsync_len	= 6,

		.de_active = 1,
		.hsync_active = 0,
		.vsync_active = 0,
		.pixelclk_active = 0,

		.hdmi_monitor = 0,
	},
	{
		.name	= "ATK-MD1010R-1280x800",
		.option = "ALIENTEK ATK-MD1010R 1280x800 60Hz",

		.pixelclock	= 71100000,

		.hactive	= 1280,
		.hfront_porch		= 70,
		.hback_porch		= 80,
		.hsync_len	= 10,

		.vactive	= 800,
		.vfront_porch	= 10,
		.vback_porch	= 10,
		.vsync_len	= 3,

		.de_active = 1,
		.hsync_active = 0,
		.vsync_active = 0,
		.pixelclk_active = 0,

		.hdmi_monitor = 0,
	},
	{
		.name	= "ATK-MD0430R-800x480",
		.option = "ALIENTEK ATK-MD0430R 800x480 60Hz",

		.pixelclock	= 27000000,

		.hactive	= 800,
		.hfront_porch		= 48,
		.hback_porch		= 40,
		.hsync_len	= 40,

		.vactive	= 480,
		.vfront_porch	= 13,
		.vback_porch	= 32,
		.vsync_len	= 0,

		.de_active = 1,
		.hsync_active = 0,
		.vsync_active = 0,
		.pixelclk_active = 0,

		.hdmi_monitor = 0,
	},
	{
		.name	= "ATK-MD0700R-1024x600",
		.option = "ALIENTEK ATK-MD0700R 1024x600 60Hz",

		.pixelclock	= 51200000,

		.hactive	= 1024,
		.hfront_porch		= 160,
		.hback_porch		= 140,
		.hsync_len	= 20,

		.vactive	= 600,
		.vfront_porch	= 12,
		.vback_porch	= 20,
		.vsync_len	= 3,

		.de_active = 1,
		.hsync_active = 0,
		.vsync_active = 0,
		.pixelclk_active = 0,

		.hdmi_monitor = 0,
	},
};

static char* option_list[ARRAY_SIZE(display_timing_mimics_set)] = {NULL,};

static int display_timing_mimics_count = ARRAY_SIZE(display_timing_mimics_set);

static inline int ls_fill_timing_entry_fix_value(struct timing_entry* entry, u32 value)
{
	if (!entry)
		return -EINVAL;

	entry->min = value;
	entry->typ = value;
	entry->max = value;

	return 0;
}

static int ls_fill_display_timing_by_mimics(struct display_timing* timing, struct display_timing_mimics_dts* mimics)
{
	if (!timing || !mimics)
		return -EINVAL;

	ls_fill_timing_entry_fix_value(&timing->pixelclock, mimics->pixelclock);

	ls_fill_timing_entry_fix_value(&timing->hactive, mimics->hactive);
	ls_fill_timing_entry_fix_value(&timing->hfront_porch, mimics->hfront_porch);
	ls_fill_timing_entry_fix_value(&timing->hback_porch, mimics->hback_porch);
	ls_fill_timing_entry_fix_value(&timing->hsync_len, mimics->hsync_len);

	ls_fill_timing_entry_fix_value(&timing->vactive, mimics->vactive);
	ls_fill_timing_entry_fix_value(&timing->vfront_porch, mimics->vfront_porch);
	ls_fill_timing_entry_fix_value(&timing->vback_porch, mimics->vback_porch);
	ls_fill_timing_entry_fix_value(&timing->vsync_len, mimics->vsync_len);

	timing->flags = 0;

	timing->flags |= mimics->de_active ? DISPLAY_FLAGS_DE_HIGH : DISPLAY_FLAGS_DE_LOW;
	timing->flags |= mimics->hsync_active ? DISPLAY_FLAGS_HSYNC_HIGH : DISPLAY_FLAGS_HSYNC_LOW;
	timing->flags |= mimics->vsync_active ? DISPLAY_FLAGS_VSYNC_HIGH : DISPLAY_FLAGS_VSYNC_LOW;
	timing->flags |= mimics->pixelclk_active ? DISPLAY_FLAGS_PIXDATA_POSEDGE : DISPLAY_FLAGS_PIXDATA_NEGEDGE;

	timing->hdmi_monitor = mimics->hdmi_monitor;

	return 0;
}

int ls_fill_display_timing_by_name(struct display_timing* timing, char* name)
{
	int i;

	if (!name)
		return -EINVAL;

	for (i = 0; i < display_timing_mimics_count; ++i) {
		if (!strcmp(name, display_timing_mimics_set[i].name))
			return ls_fill_display_timing_by_mimics(timing, display_timing_mimics_set + i);
	}

	return -EINVAL;
}

static inline void generic_all_panel_option(void)
{
	int i;
	int len;
	struct display_timing_mimics_dts* mimics;
	// 已经初始化过了
	if (likely(option_list[0]))
		return;

	for (i = 0; i < display_timing_mimics_count; ++i) {
		mimics = display_timing_mimics_set + i;
		len = strlen(mimics->name) + strlen(mimics->option) + 1;
		option_list[i] = (char*)calloc(len + 1, sizeof(char));
		sprintf(option_list[i], "%s=%s", mimics->option, mimics->name);
	}
}

#define LS_DT_DEAFULT_OPTION_DESC "use board default panel"
#define LS_DT_DEFAULT_OPTION LS_DT_DEAFULT_OPTION_DESC"=default"

char* ls_panel_display_timing_option_generic(int n)
{
	if (n > display_timing_mimics_count || n < 0)
		return NULL;

	// use dts default desc panel
	if (n == display_timing_mimics_count)
		return LS_DT_DEFAULT_OPTION;

	generic_all_panel_option();

	return option_list[n];
}

char* ls_panel_display_timing_option_desc(int n)
{
	if (n > display_timing_mimics_count || n < 0)
		return NULL;

	// use dts default desc panel
	if (n == display_timing_mimics_count)
		return LS_DT_DEAFULT_OPTION_DESC;

	return display_timing_mimics_set[n].option;
}

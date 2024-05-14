#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <init.h>
#include <ram.h>
#include <asm/io.h>
#include <config.h>
#include <stdio.h>
#include "ls_addr4.h"

#ifdef DDR_DEBUG
#define mem_debug	printf
#else
#define mem_debug(s...)
#endif

#define	NVRAM_SIZE		494
#define	NVRAM_SECSIZE		500
#define	NVRAM_OFFS		0x000ff000	/* erase ops is align with 4K, Set in common use */

#define	DTB_SIZE 0x4000		/* 16K dtb size*/
#define	DTB_OFFS		(NVRAM_OFFS - DTB_SIZE)
#define DIMM_INFO_IN_FLASH_OFFS	(PHYS_TO_UNCACHED(0x1c000000) + DTB_OFFS - 0x1000)

#define USE_DDR_1TMODE
#define MC_PER_NODE 		1
#define HIGH_MEM_WIN_BASE_ADDR 	0x80000000
#define DDR_CFG_BASE		PHYS_TO_UNCACHED(0xff00000)

enum {
	MC_ID_2K0300 = 0,
};


ddr_ctrl mm_ctrl_info;
extern int ddr4_init (uint64_t node_num, ddr_ctrl *mm_ctrl);

#if defined(CONFIG_SPL_BUILD) || !defined(CONFIG_SPL)

uint64_t get_hex(void)
{
	uint8_t c, cnt = 16;
	uint64_t val = 0;
	while (1) {
		c = getchar();

		switch (c) {
		case 'q':
		case ' ':
		case '\r':
			/* abort in advance */
			return val;
		case '\b':
			if(cnt < 16) {
				cnt++;
				val = val >> 4;
				// putchar('\b');
				// putchar(' ');
				// putchar('\b');
			}
		default:
			if (!cnt)
				continue;
			if (c >= '0' && c <= '9') {
				val = val << 4 | (c - '0');
			} else if (c >= 'a' && c <= 'f') {
				val = val << 4 | (c - 'a' + 10);
			} else if (c >= 'A' && c <= 'F') {
				val = val << 4 | (c - 'A' + 10);
			} else {
				continue;
			}
			// putchar(c);
			cnt--;
		}
	}
}

void mm_feature_init(void)
{
	mm_ctrl_info.mc_type = LS2K0300_MC_TYPE;
#ifdef LOONGSON_3D5000
	mm_ctrl_info.mc_interleave_offset = 12;
#elif LOONGSON_3A6000
	mm_ctrl_info.mc_interleave_offset = 13;
#else
	mm_ctrl_info.mc_interleave_offset = 8;
#endif
	mm_ctrl_info.mc_regs_base = DDR_CFG_BASE;
	mm_ctrl_info.cache_mem_base = 0xa000000000000000;
	mm_ctrl_info.dimm_info_in_flash_offset = DIMM_INFO_IN_FLASH_OFFS;
	mm_ctrl_info.ddr_freq = DDR_FREQ;
	mm_ctrl_info.ddr_freq_2slot = 400;//DDR_FREQ_2SLOT;
	mm_ctrl_info.node_offset = 44;//NODE_OFFSET;
	mm_ctrl_info.tot_node_num = TOT_NODE_NUM;
	mm_ctrl_info.node_mc_num = MC_PER_NODE;
	mm_ctrl_info.ref_clk = 100;
	mm_ctrl_info.spi_base = SPI_BASE;
	mm_ctrl_info.uart_base = UART_BASE;
	mm_ctrl_info.l2xbar_conf_addr = L2XBAR_CONF_ADDR;
	mm_ctrl_info.channel_width = 64;
	mm_ctrl_info.dll_bypass = 0;
	//if you want change kernel high start address you should change the macro
	mm_ctrl_info.mem_base = HIGH_MEM_WIN_BASE_ADDR;
	/* mm_ctrl is global variable */
	mm_ctrl_info.table.enable_early_printf		= 0;
#ifdef DDR3_MODE
	mm_ctrl_info.table.ddr_param_store		= 0;
	mm_ctrl_info.table.ddr3_dimm			= 1;
	mm_ctrl_info.table.enable_mc_vref_training	= 0;
	mm_ctrl_info.table.enable_ddr_vref_training	= 0;
	mm_ctrl_info.table.enable_bit_training		= 0;
	mm_ctrl_info.table.disable_dimm_ecc		= 1;
#else
	mm_ctrl_info.table.ddr_param_store		= 1;
	mm_ctrl_info.table.ddr3_dimm			= 0;
	mm_ctrl_info.table.enable_mc_vref_training	= 1;
	mm_ctrl_info.table.enable_ddr_vref_training	= 1;
	mm_ctrl_info.table.enable_bit_training		= 0;
	mm_ctrl_info.table.disable_dimm_ecc		= 1;
#endif
	mm_ctrl_info.table.low_speed			= 0;
	mm_ctrl_info.table.auto_ddr_config		= 0;
	mm_ctrl_info.table.enable_ddr_leveling		= 1;
	mm_ctrl_info.table.print_ddr_leveling		= 0;
	mm_ctrl_info.table.vref_training_debug		= 0;
	mm_ctrl_info.table.bit_training_debug		= 1;
	mm_ctrl_info.table.enable_write_training	= 1;
	mm_ctrl_info.table.debug_write_training		= 0;
	mm_ctrl_info.table.print_dll_sample		= 0;
	mm_ctrl_info.table.disable_dq_odt_training	= 1;
	mm_ctrl_info.table.lvl_debug			= 0;
	mm_ctrl_info.table.disable_dram_crc		= 1;
	mm_ctrl_info.table.two_t_mode_enable		= 0;
	mm_ctrl_info.table.disable_read_dbi		= 1;
	mm_ctrl_info.table.disable_write_dbi		= 1;
	mm_ctrl_info.table.disable_dm			= 0;
	mm_ctrl_info.table.preamble2			= 0;
	mm_ctrl_info.table.set_by_protocol		= 1;
	mm_ctrl_info.table.param_set_from_spd_debug	= 0;
	mm_ctrl_info.table.refresh_1x			= 1;
	mm_ctrl_info.table.spd_only			= 0;
	mm_ctrl_info.table.ddr_debug_param		= 0;
	mm_ctrl_info.table.ddr_soft_clksel		= 1;
#ifdef LS_STR
	mm_ctrl_info.table.str				= 1;
#else
	mm_ctrl_info.table.str				= 0;
#endif
#ifdef DDR3_MODE
	mm_ctrl_info.vref.vref				= 0x78;
	mm_ctrl_info.table.pda_mode			= 0;
#else
	mm_ctrl_info.vref.vref				= 0x50;
	mm_ctrl_info.table.pda_mode			= 1;
#endif
	mm_ctrl_info.table.signal_test			= 0;
	mm_ctrl_info.vref.mc_vref_adjust		= 0x0;
	mm_ctrl_info.vref.ddr_vref_adjust		= 0x0;
	mm_ctrl_info.vref.vref_init			= 0x20;
	mm_ctrl_info.data.rl_manualy			= 0;
	mm_ctrl_info.data.bit_width			= 16;
	mm_ctrl_info.data.nc16_map			= 0;
	mm_ctrl_info.data.gate_mode			= 0;
	
	mm_ctrl_info.data.pad_reset_po                  = 0x0;

	mm_ctrl_info.data.wrlevel_count_low             = 0x0;
	mm_ctrl_info.vref.vref_bits_per                 = 0x0;
	mm_ctrl_info.vref.vref_bit 	                = 0x0;
	mm_ctrl_info.data.ref_manualy	                = 0x0;

	mm_ctrl_info.param.dll_ck_mc0			= 0x44;
	mm_ctrl_info.param.dll_ck_mc1			= 0x44;
#ifdef LOONGSON_3C5000
	mm_ctrl_info.param.dll_ck_mc2			= 0x44;
	mm_ctrl_info.param.dll_ck_mc3			= 0x44;
#endif
	mm_ctrl_info.param.RCD					= 0;
	mm_ctrl_info.param.RP					= 0;
	mm_ctrl_info.param.RAS					= 0;
	mm_ctrl_info.param.REF					= 0;
	mm_ctrl_info.param.RFC					= 0;

	mm_ctrl_info.ocd.pad_clk_ocd			= PAD_CLK_OCD;
	mm_ctrl_info.ocd.pad_ctrl_ocd			= PAD_CTRL_OCD;
	mm_ctrl_info.ocd.pad_ds_split			= PAD_DS_SPLIT_ALL;

	mm_ctrl_info.odt.rtt_nom_1r_1slot		= RTT_NOM;
	mm_ctrl_info.odt.rtt_park_1r_1slot		= RTT_PARK;
	mm_ctrl_info.odt.mc_dqs_odt_1cs			= MC_DQS_ODT;
	mm_ctrl_info.odt.mc_dq_odt_1cs			= MC_DQ_ODT;

	mm_ctrl_info.odt.rtt_nom_2r_1slot		= RTT_NOM_2RANK;
	mm_ctrl_info.odt.rtt_park_2r_1slot		= RTT_PARK_2RANK;

	mm_ctrl_info.odt.rtt_nom_1r_2slot_cs0		= RTT_NOM_CS0;
	mm_ctrl_info.odt.rtt_park_1r_2slot_cs0		= RTT_PARK_CS0;
	mm_ctrl_info.odt.rtt_nom_1r_2slot_cs1		= RTT_NOM_CS1;
	mm_ctrl_info.odt.rtt_park_1r_2slot_cs1		= RTT_PARK_CS1;

	mm_ctrl_info.odt.rtt_nom_2r_2slot_cs0		= RTT_NOM_2R_CS0;
	mm_ctrl_info.odt.rtt_park_2r_2slot_cs0		= RTT_PARK_2R_CS0;
	mm_ctrl_info.odt.rtt_nom_2r_2slot_cs2		= RTT_NOM_2R_CS2;
	mm_ctrl_info.odt.rtt_park_2r_2slot_cs2		= RTT_PARK_2R_CS2;

	mm_ctrl_info.odt.mc_dqs_odt_2cs			= MC_DQS_ODT_2CS;
	mm_ctrl_info.odt.mc_dq_odt_2cs			= MC_DQ_ODT_2CS;

	mm_ctrl_info.sameba_adj				= MC_PHY_REG_DATA_070;
	mm_ctrl_info.samebg_adj				= MC_PHY_REG_DATA_078;
	mm_ctrl_info.samec_adj				= MC_PHY_REG_DATA_080;
	mm_ctrl_info.samecs_adj				= MC_PHY_REG_DATA_090;
	mm_ctrl_info.diffcs_adj				= MC_PHY_REG_DATA_098;

	/* paster parameter */
	mm_ctrl_info.paster.mc0_enable			= MC0_ENABLE;
	mm_ctrl_info.paster.mc1_enable			= MC1_ENABLE;

	mm_ctrl_info.paster.mc0_memsize			= MC0_MEMSIZE;
	mm_ctrl_info.paster.mc0_dram_type		= MC0_DRAM_TYPE;
	mm_ctrl_info.paster.mc0_dimm_type		= MC0_DIMM_TYPE;
	mm_ctrl_info.paster.mc0_module_type		= MC0_MODULE_TYPE;
	mm_ctrl_info.paster.mc0_cid_num			= MC0_CID_NUM;
	mm_ctrl_info.paster.mc0_ba_num			= MC0_BA_NUM;
	mm_ctrl_info.paster.mc0_bg_num			= MC0_BG_NUM;
	mm_ctrl_info.paster.mc0_csmap			= MC0_CSMAP;
	mm_ctrl_info.paster.mc0_dram_width		= MC0_DRAM_WIDTH;
	mm_ctrl_info.paster.mc0_module_width		= MC0_MODULE_WIDTH;
	mm_ctrl_info.paster.mc0_sdram_capacity		= MC0_SDRAM_CAPACITY;
	mm_ctrl_info.paster.mc0_col_num			= MC0_COL_NUM;
	mm_ctrl_info.paster.mc0_row_num			= MC0_ROW_NUM;
	mm_ctrl_info.paster.mc0_addr_mirror		= MC0_ADDR_MIRROR;
	mm_ctrl_info.paster.mc0_bg_mirror		= MC0_BG_MIRROR;

	mm_ctrl_info.paster.mc1_memsize			= MC1_MEMSIZE;
	mm_ctrl_info.paster.mc1_dram_type		= MC1_DRAM_TYPE;
	mm_ctrl_info.paster.mc1_dimm_type		= MC1_DIMM_TYPE;
	mm_ctrl_info.paster.mc1_module_type		= MC1_MODULE_TYPE;
	mm_ctrl_info.paster.mc1_cid_num			= MC1_CID_NUM;
	mm_ctrl_info.paster.mc1_ba_num			= MC1_BA_NUM;
	mm_ctrl_info.paster.mc1_bg_num			= MC1_BG_NUM;
	mm_ctrl_info.paster.mc1_csmap			= MC1_CSMAP;
	mm_ctrl_info.paster.mc1_dram_width		= MC1_DRAM_WIDTH;
	mm_ctrl_info.paster.mc1_module_width		= MC1_MODULE_WIDTH;
	mm_ctrl_info.paster.mc1_sdram_capacity		= MC1_SDRAM_CAPACITY;
	mm_ctrl_info.paster.mc1_col_num			= MC1_COL_NUM;
	mm_ctrl_info.paster.mc1_row_num			= MC1_ROW_NUM;
	mm_ctrl_info.paster.mc1_addr_mirror		= MC1_ADDR_MIRROR;
	mm_ctrl_info.paster.mc1_bg_mirror		= MC1_BG_MIRROR;
	mm_ctrl_info.param_reg_array			= &param_info;
}

void print_mm_ctrl()
{
	printf("=================================================================================\n");
	printf(" mc_type:%u\n",						 mm_ctrl_info.mc_type);
	printf(" mc_regs_base:%llu\n",                   mm_ctrl_info.mc_regs_base);
	printf(" cache_mem_base:%llu\n",                 mm_ctrl_info.cache_mem_base);
	printf(" mem_base:%llu\n",                       mm_ctrl_info.mem_base);
	printf(" ddr_freq:%lu\n",                       mm_ctrl_info.ddr_freq);
	printf(" ddr_freq_2slot:%lu\n",                 mm_ctrl_info.ddr_freq_2slot);
	printf(" dimm_info_in_flash_offset:%lu\n",      mm_ctrl_info.dimm_info_in_flash_offset);
	printf(" sameba_adj:%lu\n",                     mm_ctrl_info.sameba_adj);
	printf(" samebg_adj:%lu\n",                     mm_ctrl_info.samebg_adj);
	printf(" samec_adj:%lu\n",                      mm_ctrl_info.samec_adj);
	printf(" samecs_adj:%lu\n",                     mm_ctrl_info.samecs_adj);
	printf(" diffcs_adj:%lu\n",                     mm_ctrl_info.diffcs_adj);
	printf(" mc_interleave_offset:%llu\n",           mm_ctrl_info.mc_interleave_offset);
	printf(" ref_clk:%u\n",                        mm_ctrl_info.ref_clk);
	printf("node_offset:%u\n",                     mm_ctrl_info.node_offset);
	printf("tot_node_num:%u\n",                    mm_ctrl_info.tot_node_num);
	printf("channel_width:%u\n",                   mm_ctrl_info.channel_width);
	printf("dll_bypass:%u\n",                      mm_ctrl_info.dll_bypass);
	printf("=================================================================================\n");
}

void mc_init(struct mc_setting *mc)
{
	return;
}
#endif


static int ls_ddr_parse_dt(struct udevice *dev)
{
	#if 0
	struct mc_setting *mc = dev_get_priv(dev);
	const char *str;
	u32 val;
	int mc_id, ret;
	u32 sdram_rows, sdram_cols, sdram_banks;
	u32 sdram_width, sdram_data_width;
	size_t sdram_size;

	mc_id = dev->driver_data;

	ret = dev_read_u32(dev, "soc-config-reg", &val);
	if (ret) {
		debug("ddr: get soc-config-reg failed!\n");
		return ret;
	}
	mc->soc_cfg_base = (void*)PHYS_TO_UNCACHED(val);

	ret = dev_read_u32(dev, "mc-disable-offs", &val);
	if (ret) {
		debug("ddr: get mc-disable-offs failed!\n");
		return ret;
	}
	mc->mc_disable_offs = val;

	ret = dev_read_u32(dev, "mc-default-offs", &val);
	if (ret) {
		debug("ddr: get mc-default-offs failed!\n");
		return ret;
	}
	mc->mc_default_offs = val;

	str = dev_read_string(dev, "sdram-type");
	if (str && !strcmp(str, "ddr2")) {
		mc->sdram_type = MC_SDRAM_TYPE_DDR2;
	} else if (str && !strcmp(str, "ddr3")) {
		mc->sdram_type = MC_SDRAM_TYPE_DDR3;
	} else {
		mc->sdram_type = MC_SDRAM_TYPE_DDR3;
	}

	str = dev_read_string(dev, "dimm-type");
	if (str && !strcmp(str, "rdimm")) {
		mc->dimm_type = MC_DIMM_TYPE_RDIMM;
	} else if (str && !strcmp(str, "sodimm")) {
		mc->dimm_type = MC_DIMM_TYPE_SODIMM;
	} else {
		mc->dimm_type = MC_DIMM_TYPE_UDIMM;
	}

	ret = dev_read_u32(dev, "cs-map", &val);
	if (ret) {
		debug("ddr: get cs-map failed!\n");
		return ret;
	}
	mc->cs_map = val;

	ret = dev_read_u32(dev, "data-width", &val);
	if (ret) {
		debug("ddr: get data-width failed!\n");
		return ret;
	}
	sdram_data_width = val;
	switch (val)
	{
	case 16:
		mc->data_width = MC_DATA_WIDTH_16;
		mc->slice_num = 2;
		break;
	case 32:
		mc->data_width = MC_DATA_WIDTH_32;
		mc->slice_num = 4;
		break;
	case 64:
		mc->data_width = MC_DATA_WIDTH_64;
		mc->slice_num = 8;
		break;
	default:
		debug("ddr: data width error!\n");
		return -EINVAL;
	}

	ret = dev_read_u32(dev, "sdram-width", &val);
	if (ret) {
		debug("ddr: get sdram-width failed!\n");
		return ret;
	}
	sdram_width = val;
	switch (val)
	{
	case 8:
		mc->sdram_width = MC_SDRAM_WIDTH_X8;
		break;
	case 16:
		mc->sdram_width = MC_SDRAM_WIDTH_X16;
		break;
	default:
		debug("ddr: sdram width error!\n");
		return -EINVAL;
	}

	ret = dev_read_u32(dev, "sdram-banks", &val);
	if (ret) {
		debug("ddr: get sdram-banks failed!\n");
		return ret;
	}
	sdram_banks = val;
	switch (val)
	{
	case 2:
		mc->sdram_banks = MC_SDRAM_BANK_2;
		break;
	case 4:
		mc->sdram_banks = MC_SDRAM_BANK_4;
		break;
	case 8:
		mc->sdram_banks = MC_SDRAM_BANK_8;
		break;
	default:
		debug("ddr: sdram banks error!\n");
		return -EINVAL;
	}

	ret = dev_read_u32(dev, "sdram-rows", &val);
	if (ret) {
		debug("ddr: get sdram-rows failed!\n");
		return ret;
	}
	sdram_rows = val;
	mc->sdram_rows = MC_SDRAM_ROW(val);

	ret = dev_read_u32(dev, "sdram-cols", &val);
	if (ret) {
		debug("ddr: get sdram-cols failed!\n");
		return ret;
	}
	sdram_cols = val;
	mc->sdram_cols = MC_SDRAM_COL(val);

	if (dev_read_bool(dev, "ecc-enable"))
		mc->ecc_en = MC_ECC_EN_YES;

	if (dev_read_bool(dev, "addr-mirror"))
		mc->addr_mirror = true;

	if (dev_read_bool(dev, "multi-channel"))
		mc->multi_channel = true;

	if (dev_read_bool(dev, "reset-revert"))
		mc->reset_revert = true;

	ret = dev_read_u32(dev, "clk-latency", &val);
	if (!ret)
		mc->clk_latency = val;

	ret = dev_read_u32(dev, "rd-latency", &val);
	if (!ret)
		mc->rd_latency = val;

	ret = dev_read_u32(dev, "wr-latency", &val);
	if (!ret)
		mc->wr_latency = val;

	ret = dev_read_u32(dev, "cmd-timing-mode", &val);
	if (!ret) {
		if (val == 0)
			mc->cmd_timing_mode = MC_CMD_TIMING_MODE_1T;
		else if (val == 1)
			mc->cmd_timing_mode = MC_CMD_TIMING_MODE_2T;
		else if (val == 2)
			mc->cmd_timing_mode = MC_CMD_TIMING_MODE_3T;
		else
			mc->cmd_timing_mode = MC_CMD_TIMING_MODE_2T;
	}

	/*每个内存颗粒的大小*/
	sdram_size = (1 << sdram_rows) * (1 << sdram_cols)
					* sdram_banks * (sdram_width / 8);
	/*总内存的大小*/
	mc->info.size = sdram_size * (sdram_data_width / sdram_width);
#endif
	return 0;
}

static int ls_ddr_probe(struct udevice *dev)
{
	struct mc_setting *mc = dev_get_priv(dev);

	if (ls_ddr_parse_dt(dev)) {
		printf("ddr: parse device tree failed!\n");
		return -EINVAL;
	}

#if defined(CONFIG_SPL_BUILD) || !defined(CONFIG_SPL)
	/* ddr soft rst */
	readl(L2XBAR_CONFIG_BASE_ADDR + 0x11c) &= ~0x1;
	readl(L2XBAR_CONFIG_BASE_ADDR + 0x11c) |= 0x1;

	mm_feature_init();
	ddr4_init(TOT_NODE_NUM, &mm_ctrl_info);
	// mc->mm_ctrl_info = &mm_ctrl_info;
	mc->info.size = MC0_MEMSIZE * 1024 * 1024;
#else
	printf("%ld MiB\n", mc->info.size >> 20); // 这里是 DRAM: 打印之后输出 这里可以把全部的内存大小打印出来
#endif

	return 0;
}

static int ls_ddr_get_info(struct udevice *dev, struct ram_info *info)
{
	struct mc_setting *priv = dev_get_priv(dev);

	*info = priv->info;

	return 0;
}

static struct ram_ops ls_ddr_ops = {
	.get_info = ls_ddr_get_info,
};

static const struct udevice_id ls_ddr_ids[] = {
	{ .compatible = "loongson,ls2k0300-ddr", .data = MC_ID_2K0300 },
	{ }
};

U_BOOT_DRIVER(ls_ddr) = {
	.name = "ls_ddr",
	.id = UCLASS_RAM,
	.of_match = ls_ddr_ids,
	.ops = &ls_ddr_ops,
	.probe = ls_ddr_probe,
	.priv_auto = sizeof(struct mc_setting),
};

#ifndef _MEM_CTRL_H
#define _MEM_CTRL_H

#include <linux/types.h>
// #include <asm-generic/types.h>

#define tR2R_sameba_adj (0x2ULL & 0x3f)
#define tR2W_sameba_adj (0x2ULL & 0x3f)
#define tR2P_sameba_adj (0x0ULL & 0x3f)
#define tW2R_sameba_adj (0x2ULL & 0x3f)
#define tW2W_sameba_adj (0x2ULL & 0x3f)
#define tW2P_sameba_adj (0x0ULL & 0x3f)
#ifdef LOONGSON_3A6000
#define tR2R_samebg_adj (0x0ULL & 0x3f)
#else
#define tR2R_samebg_adj (0x2ULL & 0x3f)
#endif
#define tR2W_samebg_adj (0x4ULL & 0x3f)
#define tR2P_samebg_adj (0x0ULL & 0x3f)
#define tW2R_samebg_adj (0x2ULL & 0x3f)
#define tW2W_samebg_adj (0x2ULL & 0x3f)
#define tW2P_samebg_adj (0x0ULL & 0x3f)
#define tR2R_samec_adj  (0x2ULL & 0x3f)
#define tR2W_samec_adj  (0x4ULL & 0x3f)
#define tR2P_samec_adj  (0x0ULL & 0x3f)
#define tW2R_samec_adj  (0x2ULL & 0x3f)
#define tW2W_samec_adj  (0x2ULL & 0x3f)
#define tW2P_samec_adj  (0x0ULL & 0x3f)
#define tR2R_samecs_adj (0x2ULL & 0x3f)
#ifdef LOONGSON_3A6000
#define tR2W_samecs_adj (0x4ULL & 0x3f)
#else
#define tR2W_samecs_adj (0x0ULL & 0x3f)
#endif
#define tR2P_samecs_adj (0x0ULL & 0x3f)
#define tW2R_samecs_adj (0x2ULL & 0x3f)
#define tW2W_samecs_adj (0x2ULL & 0x3f)
#define tW2P_samecs_adj (0x0ULL & 0x3f)
#define tR2R_diffcs_adj (0x4ULL & 0x3f)
#define tR2W_diffcs_adj (0x4ULL & 0x3f)
#define tR2P_diffcs_adj (0x0ULL & 0x3f)
#define tW2R_diffcs_adj (0x4ULL & 0x3f)
#define tW2W_diffcs_adj (0x4ULL & 0x3f)
#define tW2P_diffcs_adj (0x0ULL & 0x3f)

#define PAD_CLK_OCD    0x5 // max 7
#define PAD_CTRL_OCD   0xe // max f
#define PAD_DS_DCC     0x8ULL
#define PAD_DS_OCD_DQ  0x7ULL
#define PAD_DS_OCD_DQS 0x7ULL//drive strength:max7
#define PAD_DS_SPLIT ((PAD_DS_DCC<<12)|(PAD_DS_OCD_DQ<<3)|(PAD_DS_OCD_DQS))
#define PAD_DS_SPLIT_ALL (PAD_DS_SPLIT<<48)|(PAD_DS_SPLIT<<32)|(PAD_DS_SPLIT<<16)|(PAD_DS_SPLIT)

#define TOT_NODE_NUM 		1
#define SPI_BASE      0x16010000
#define UART_BASE       PHYS_TO_UNCACHED(0x16100000)
#define CHIP_CONFIG_OFFSET      0x100
#define L2XBAR_CONFIG_BASE_ADDR     PHYS_TO_UNCACHED(0x16000000)
#define L2XBAR_CONF_ADDR  L2XBAR_CONFIG_BASE_ADDR | CHIP_CONFIG_OFFSET

//define ODT value
#define RTT_240         4
#define RTT_120         2
#define RTT_80          6
#define RTT_60          1
#define RTT_48          5
#define RTT_40          3
#define RTT_34          7
#define RTT_DISABLE     0

#define RTT_NOM_7A      RTT_40
#define RTT_PARK_7A     RTT_120
//1rank 1slot
#ifdef LOONGSON_3D5000
#define RTT_NOM     RTT_40
#define MC_DQ_ODT   3
#elif LOONGSON_3A6000
#define RTT_NOM     RTT_60
#define MC_DQ_ODT   2
#else
#define RTT_NOM     RTT_40
#define MC_DQ_ODT   4
#endif
#define RTT_PARK    RTT_120
#define MC_DQS_ODT  8

//2rank 1slot
#define RTT_NOM_2RANK   RTT_120
#define RTT_PARK_2RANK  RTT_48

//1rank 2slot
#define RTT_NOM_CS0     RTT_80
#define RTT_NOM_CS1     RTT_120
#define RTT_PARK_CS0    RTT_40
#define RTT_PARK_CS1    RTT_60

//2rank 2slot
#define RTT_NOM_2R_CS0		RTT_120
#define RTT_NOM_2R_CS2		RTT_120
#define RTT_PARK_2R_CS0		RTT_48
#define RTT_PARK_2R_CS2		RTT_60

//2cs or 4cs
#define MC_DQS_ODT_2CS  5
#define MC_DQ_ODT_2CS   4

#define MC_PHY_REG_DATA_070 (tW2P_sameba_adj<<40)|(tW2W_sameba_adj<<32)|(tW2R_sameba_adj<<24)|(tR2P_sameba_adj<<16)|(tR2W_sameba_adj<<8)|(tR2R_sameba_adj); //070
#define MC_PHY_REG_DATA_078 (tW2P_samebg_adj<<40)|(tW2W_samebg_adj<<32)|(tW2R_samebg_adj<<24)|(tR2P_samebg_adj<<16)|(tR2W_samebg_adj<<8)|(tR2R_samebg_adj); //078
#define MC_PHY_REG_DATA_080 (tW2P_samec_adj<<40) |(tW2W_samec_adj<<32) |(tW2R_samec_adj<<24) |(tR2P_samec_adj<<16) |(tR2W_samec_adj<<8) |(tR2R_samec_adj);  //080
#define MC_PHY_REG_DATA_090 (tW2P_samecs_adj<<40)|(tW2W_samecs_adj<<32)|(tW2R_samecs_adj<<24)|(tR2P_samecs_adj<<16)|(tR2W_samecs_adj<<8)|(tR2R_samecs_adj); //090
#define MC_PHY_REG_DATA_098 (tW2P_diffcs_adj<<40)|(tW2W_diffcs_adj<<32)|(tW2R_diffcs_adj<<24)|(tR2P_diffcs_adj<<16)|(tR2W_diffcs_adj<<8)|(tR2R_diffcs_adj); //098

typedef union {
	struct {
		uint64_t enable_early_printf		: 1;
		uint64_t ddr_param_store		: 1;
		uint64_t ddr3_dimm			: 1;
		uint64_t low_speed			: 1;
		uint64_t auto_ddr_config		: 1;
		uint64_t enable_ddr_leveling		: 1;
		uint64_t print_ddr_leveling		: 1;
		uint64_t enable_mc_vref_training	: 1;
		uint64_t vref_training_debug		: 1;
		uint64_t enable_ddr_vref_training	: 1;
		uint64_t enable_bit_training		: 1;
		uint64_t bit_training_debug		: 1;
		uint64_t enable_write_training		: 1;
		uint64_t debug_write_training		: 1;
		uint64_t print_dll_sample		: 1;
		uint64_t disable_dq_odt_training	: 1;
		uint64_t lvl_debug			: 1;
		uint64_t disable_dram_crc		: 1;
		uint64_t two_t_mode_enable		: 1;
		uint64_t disable_dimm_ecc		: 1;
		uint64_t disable_read_dbi		: 1;
		uint64_t disable_write_dbi		: 1;
		uint64_t disable_dm			: 1;
		uint64_t preamble2			: 1;
		uint64_t set_by_protocol		: 1;
		uint64_t param_set_from_spd_debug	: 1;
		uint64_t refresh_1x			: 1;
		uint64_t spd_only			: 1;
		uint64_t ddr_debug_param		: 1;
		uint64_t ddr_soft_clksel		: 1;
		uint64_t str				: 1;
		uint64_t pda_mode			: 1;
		uint64_t signal_test			: 1;
	};
	uint64_t feature;
} ddr_feature;

#define DDR_PARAM_STORE(x)		if (x->table.ddr_param_store		) {
#define DDR3_DIMM(x)			if (x->table.ddr3_dimm			) {
#define AUTO_DDR_CONFIG(x)		if (x->table.auto_ddr_config		) {
#define ENABLE_DDR_LEVELING(x)		if (x->table.enable_ddr_leveling	) {
#define PRINT_DDR_LEVELING(x)		if (x->table.print_ddr_leveling		) {
#define ENABLE_MC_VREF_TRAINING(x)	if (x->table.enable_mc_vref_training	) {
#define VREF_TRAINING_DEBUG(x)		if (x->table.vref_training_debug	) {
#define ENABLE_DDR_VREF_TRAINING(x)	if (x->table.enable_ddr_vref_training	) {
#define ENABLE_BIT_TRAINING(x)		if (x->table.enable_bit_training	) {
#define BIT_TRAINING_DEBUG(x)		if (x->table.bit_training_debug		) {
#define PRINT_DLL_SAMPLE(x)		if (x->table.print_dll_sample		) {
#define DISABLE_DQ_ODT_TRAINING(x)	if (x->table.disable_dq_odt_training	) {
#define LVL_DEBUG(x)			if (x->table.lvl_debug			) {
#define DISABLE_DRAM_CRC(x)		if (x->table.disable_dram_crc		) {
#define TWO_T_MODE_ENABLE(x)		if (x->table.two_t_mode_enable		) {
#define DISABLE_DIMM_ECC(x)		if (x->table.disable_dimm_ecc		) {
#define DISABLE_READ_DBI(x)		if (x->table.disable_read_dbi		) {
#define DISABLE_WRITE_DBI(x)		if (x->table.disable_write_dbi		) {
#define DISABLE_DM(x)			if (x->table.disable_dm			) {
#define SET_BY_PROTOCOL(x)		if (x->table.set_by_protocol		) {
#define PARAM_SET_FROM_SPD_DEBUG(x)	if (x->table.param_set_from_spd_debug	) {
#define REFRESH_1X(x)			if (x->table.refresh_1x			) {
#define SPD_ONLY(x)			if (x->table.spd_only			) {
#define DEBUG_DDR_PARAM(x)		if (x->table.ddr_debug_param		) {
#define SOFT_CLKSEL(x)			if (x->table.ddr_soft_clksel		) {
#define LS3A7A_MM_STR(x)		if (x->table.str			) {
#define MM_ELSE				} else {
#define MM_ENDIF			}

typedef union {
	struct {
		uint64_t rl_manualy	   : 5;
		uint64_t bit_width	   : 7;
		uint64_t nc16_map	   : 3;
		uint64_t mc_vref_adjust    : 5;
		uint64_t gate_mode	   : 2;
		uint64_t pad_reset_po      : 2;
		uint64_t wrlevel_count_low : 8;
		uint64_t ref_manualy       : 32;
	};
	uint64_t param;
} ddr_param;

typedef struct {
	uint8_t		RCD;
	uint8_t 	RP;
	uint8_t		RAS;
	uint16_t	REF;
	uint16_t	RFC;
	uint8_t		dll_ck_mc0;
	uint8_t		dll_ck_mc1;
	uint8_t		dll_ck_mc2;
	uint8_t		dll_ck_mc3;
} parameter;

typedef struct {
	char     mc_vref_adjust;
	char     ddr_vref_adjust;
	uint8_t  vref;
	uint8_t  vref_init;
	uint8_t  vref_bits_per;
	uint8_t  vref_bit;
} vref_param;

typedef union {
	struct {
		uint16_t slot0_addr	: 8;
		uint16_t slot1_addr	: 8;
	};
	uint16_t devid;
} dev_id;

typedef struct {
	uint64_t 	i2c_base;
	dev_id		devid;
} i2c_slot_param;

typedef struct {
	i2c_slot_param i2c_mc[TOT_NODE_NUM][4];
} i2c_param;

typedef union {
	struct {
		uint64_t rtt_nom_1r_1slot	: 4;
		uint64_t rtt_nom_2r_1slot	: 4;
		uint64_t rtt_nom_1r_2slot_cs0	: 4;
		uint64_t rtt_nom_1r_2slot_cs1	: 4;
		uint64_t rtt_nom_2r_2slot_cs0	: 4;
		uint64_t rtt_nom_2r_2slot_cs2	: 4;
		uint64_t rtt_park_1r_1slot	: 4;
		uint64_t rtt_park_2r_1slot	: 4;
		uint64_t rtt_park_1r_2slot_cs0	: 4;
		uint64_t rtt_park_1r_2slot_cs1	: 4;
		uint64_t rtt_park_2r_2slot_cs0	: 4;
		uint64_t rtt_park_2r_2slot_cs2	: 4;
		uint64_t mc_dqs_odt_1cs		: 4;
		uint64_t mc_dq_odt_1cs		: 4;
		uint64_t mc_dqs_odt_2cs		: 4;
		uint64_t mc_dq_odt_2cs		: 4;
	};
	uint64_t odt;
} ddr_odt;

typedef struct {
	uint64_t pad_ds_split;
	uint8_t pad_clk_ocd;
	uint8_t pad_ctrl_ocd;
} pad_ocd;

typedef struct {
	uint8_t mc0_enable;
	uint8_t mc1_enable;

	uint32_t mc0_memsize;
	uint8_t mc0_dram_type;
	uint8_t mc0_dimm_type;
	uint8_t mc0_module_type;
	uint8_t mc0_cid_num;
	uint8_t mc0_ba_num;
	uint8_t mc0_bg_num;
	uint8_t mc0_csmap;
	uint8_t mc0_dram_width;
	uint8_t mc0_module_width;
	uint8_t mc0_ecc;
	uint8_t mc0_sdram_capacity;
	uint8_t mc0_col_num;
	uint8_t mc0_row_num;
	uint8_t mc0_addr_mirror;
	uint8_t mc0_bg_mirror;

	uint32_t mc1_memsize;
	uint8_t mc1_dram_type;
	uint8_t mc1_dimm_type;
	uint8_t mc1_module_type;
	uint8_t mc1_cid_num;
	uint8_t mc1_ba_num;
	uint8_t mc1_bg_num;
	uint8_t mc1_csmap;
	uint8_t mc1_dram_width;
	uint8_t mc1_module_width;
	uint8_t mc1_ecc;
	uint8_t mc1_sdram_capacity;
	uint8_t mc1_col_num;
	uint8_t mc1_row_num;
	uint8_t mc1_addr_mirror;
	uint8_t mc1_bg_mirror;
} paster_t;

typedef struct {
	uint16_t	param_offset;
	uint64_t	param_change;
} param_array;

typedef struct {
	uint8_t			node_id;
	uint8_t			mc_id;
	param_array		*param;
} param_debug;

typedef struct ddr_config
{
	uint32_t mc_type;
	uint32_t spi_base;
	uint64_t uart_base;
	uint64_t l2xbar_conf_addr;
	uint64_t mc_regs_base;
	uint64_t cache_mem_base;
	uint64_t mem_base;
	uint64_t ddr_freq;
	uint64_t ddr_freq_2slot;
	uint64_t dimm_info_in_flash_offset;
	uint64_t sameba_adj;
	uint64_t samebg_adj;
	uint64_t samec_adj;
	uint64_t samecs_adj;
	uint64_t diffcs_adj;
	uint64_t mc_interleave_offset;
	uint16_t ref_clk;
	uint8_t node_offset;
	uint8_t tot_node_num;
	uint8_t node_mc_num;
	uint8_t	channel_width;
	uint8_t	dll_bypass;
	ddr_feature table;
	ddr_param data;
	parameter param;
	vref_param vref;
	ddr_odt odt;
	pad_ocd ocd;
	paster_t paster;
	i2c_param *i2c_node;
	param_debug *param_reg_array;		//use {0xff,0xf, NULL} as the end of array
} ddr_ctrl;

struct ddr4_smbios {
	uint64_t memsize;
	uint32_t serial_number;
	uint16_t manufacturer;
	uint16_t speed;
	uint16_t configured_speed;
	uint8_t  mc_index;
	uint8_t  slot_index;
	uint8_t  mem_type;
	uint8_t  module_type;
	uint8_t  total_width;
	uint8_t  data_width;
	uint8_t  rank;
	uint8_t  part_number[20];
};

extern ddr_ctrl mm_ctrl_info;
extern ddr_ctrl *mm_ctrl;
extern param_debug param_info[];

#define LS7A2000_MC_TYPE	0
#define LS3A5000_MC_TYPE 	4
#define LS3C5000_MC_TYPE 	5
#define LS3D5000_MC_TYPE 	6
#define LS3C5000L_MC_TYPE	7
#define LS2K1500_MC_TYPE	8
#define LS2K2000_MC_TYPE	9
#define LS2K0300_MC_TYPE	0xa

#define DDR2                        0x8
#define DDR3                        0xb
#define DDR4                        0xc
/*------------------------------------------------------------
| CID_NUM            | 2'h0    | no cid            |
|                    | 2'h1    | 2dies             |
|                    | 2'h2    | 4dies             |
|                    | 2'h3    | 8dies             |
| SDRAM_BG_SIZE      | 2'h0    | no bank group     |
|                    | 2'h1    | 2  bank group     |
|                    | 2'h2    | 4  bank group     |
| SDRAM_BA_SIZE      | 1'h0    | 4 bank            |
|                    | 1'h1    | 8 bank            |
| SDRAM_ROW_SIZE     | 3'hx    | 18-x              |
| SDRAM_COL_SIZE     | 2'hx    | 12-x              |
| ADDR_MIRROR        | 1'b1    | ADDR MIRROR       |
|                    | 1'b0    | STANDARD          |
| DIMM_MEMSIZE       |12'bx    | x*1MB             |
| DIMM_WIDTH         | 2'h0    | 8bit//not support |
|                    | 2'h1    | 16bit             |
|                    | 2'h2    | 32bit             |
|                    | 2'h3    | 64bit             |
| DIMM_TYPE          | 2'h0    | Unbuffered DIMM   |
|                    | 2'h1    | Registered DIMM   |
|                    | 2'h2    | SO-DIMM           |
|                    | 2'h3    | Load Reduced DIMM |
| SDRAM_WIDTH        | 2'h0    | x4                |
|                    | 2'h1    | x8                |
|                    | 2'h2    | x16               |
| SDRAM_TYPE         | 3'h0    | NO_DIMM           |
|                    | 3'hb    | DDR3              |
|                    | 3'hc    | DDR4              |
| MC_CSMAP           | 8'b0    | CS7-CS0           |
------------------------------------------------------------*/
#define MC0_ENABLE 1
#define MC1_ENABLE 0

#define	MC0_MEMSIZE        512
#define	MC0_DRAM_TYPE      DDR4
#define	MC0_DIMM_TYPE      0
#define	MC0_MODULE_TYPE	   2
#define	MC0_CID_NUM        0
#define	MC0_BA_NUM         0
#define	MC0_BG_NUM         1
#define	MC0_CSMAP          1
#define	MC0_DRAM_WIDTH     2
#define	MC0_MODULE_WIDTH   1
#define	MC0_SDRAM_CAPACITY 0
#define	MC0_COL_NUM        2
#define	MC0_ROW_NUM        3
#define	MC0_ADDR_MIRROR    0
#define	MC0_BG_MIRROR      0

#define	MC1_MEMSIZE        8192
#define	MC1_DRAM_TYPE      DDR4
#define	MC1_DIMM_TYPE      0
#define	MC1_MODULE_TYPE	   2
#define	MC1_CID_NUM        0
#define	MC1_BA_NUM         0
#define	MC1_BG_NUM         2
#define	MC1_CSMAP          1
#define	MC1_DRAM_WIDTH     1
#define	MC1_MODULE_WIDTH   3
#define	MC1_SDRAM_CAPACITY 0
#define	MC1_COL_NUM        2
#define	MC1_ROW_NUM        2
#define	MC1_ADDR_MIRROR    0
#define	MC1_BG_MIRROR      0

#endif /* _MEM_CTRL_H */

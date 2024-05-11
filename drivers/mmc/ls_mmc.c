#include <common.h>
#include <dm.h>
#include <mmc.h>
#include <clk.h>
#include <clock_legacy.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <asm/addrspace.h>
#include <asm/io.h>
#include <linux/io.h>

#define	SDIO_DMA_DESCADDR	PHYS_TO_UNCACHED(0x80000)
#define	SDICON			0x00
#define	SDIPRE			0x04
#define	SDICMDARG		0x08
#define	SDICMDCON		0x0c
#define	SDICMDSTA		0x10
#define	SDIRSP0			0x14
#define	SDIRSP1			0x18
#define	SDIRSP2			0x1C
#define	SDIRSP3			0x20
#define	SDIDTIMER		0x24
#define	SDIBSIZE		0x28
#define	SDIDATCON		0x2C
#define	SDIDATCNT		0x30
#define	SDIDATSTA		0x34
#define	SDIFSTA			0x38
#define	SDIINTMSK		0x3C
#define	SDIWRDAT		0x40
#define	SDISTAADD0		0x44
#define	SDISTAADD1		0x48
#define	SDISTAADD2		0x4c
#define	SDISTAADD3		0x50
#define	SDISTAADD4		0x54
#define	SDISTAADD5		0x58
#define	SDISTAADD6		0x5c
#define	SDISTAADD7		0x60
#define	SDIINTEN		0x64

#define	SDIO_DES_ADDR		(SDIO_BASE + SDIWRDAT)

#define	SDIO_CON_ENCLK		BIT(0)
#define	SDIO_CON_DIACLK		0
#define	SDIO_CON_SOFTRST	BIT(8)

#define SDIO_PRE_REVCLOCK	BIT(31)

#define	SDIO_CMDCON_CMST	BIT(8)
#define	SDIO_CMDCON_WAITRSP	BIT(9)
#define	SDIO_CMDCON_LONGRSP	BIT(10)
#define	SDIO_CMDCON_AUTOSTOP	BIT(12)
#define	SDIO_CMDCON_CHECKCRC	BIT(13)
#define	SDIO_CMDCON_SDIOEN	BIT(14)

#define	SDIO_DATCON_DTST	BIT(14)
#define	SDIO_DATCON_DMAEN	BIT(15)
#define	SDIO_DATCON_WIDEMODE	BIT(16)
#define SDIO_DATCON_WIDEMODE8B	BIT(26)

#define	SDIO_INTMSK_DATFIN	BIT(0)
#define	SDIO_INTMSK_DATTOUT	BIT(1)
#define	SDIO_INTMSK_DATCRC	BIT(2)
#define	SDIO_INTMSK_CRCSTA	BIT(3)
#define	SDIO_INTMSK_PROGERR	BIT(4)
#define	SDIO_INTMSK_SDIO	BIT(5)
#define	SDIO_INTMSK_CMDFIN	BIT(6)
#define	SDIO_INTMSK_CMDTOUT	BIT(7)
#define	SDIO_INTMSK_RSPCRC	BIT(8)
#define	SDIO_INTMSK_R1BFIN	BIT(9)

#define	SDIO_MAKE_CMD(c,f)	((c & 0x3f) | 0x140 | (f))
#define	SDIO_GET_CMDINT(c)	(c & 0x3c0)
#define	SDIO_GET_DATAINT(c)	(c & 0x1f)
#define	SDIO_GET_CMDIDX(c)	(c & 0x3f)
#define	SDIO_GET_CMD(c)		((c>>8) & 0x3f)

#define	DMA_CMD_INTMSK		BIT(0)
#define	DMA_CMD_WRITE		BIT(12)

#define	DMA_OREDER_ASK		BIT(2)
#define	DMA_OREDER_START	BIT(3)
#define	DMA_OREDER_STOP		BIT(4)
#define	DMA_ALIGNED		32
#define	DMA_ALIGNED_MSK		(~(DMA_ALIGNED - 1))


struct sdio_mmc_plat {
    struct mmc_config cfg;
    struct mmc mmc;
};

struct sdio_host {
    const char *name;
    phys_addr_t ioaddr;
    struct mmc *mmc;
    struct mmc_config *cfg;
    void *dma_desc_addr;
    void *wdma_order_addr;
    void *rdma_order_addr;
    unsigned int clock;

};

struct dma_desc {
    unsigned int order_addr_low;
    unsigned int saddr_low;
    unsigned int daddr;
    unsigned int length;
    unsigned int step_length;
    unsigned int step_times;
    unsigned int cmd;
    unsigned int order_addr_high;
    unsigned int saddr_high;
};

static inline void sdio_writel(struct sdio_host *host, int reg, u32 val)
{
	writel(val, host->ioaddr + reg);
}

static inline u32 sdio_readl(struct sdio_host *host, int reg)
{
	return readl(host->ioaddr + reg);
}

static int sdio_prepare_dma(struct sdio_host *host, struct mmc_data *data)
{
	struct dma_desc* desc = (struct dma_desc *)host->dma_desc_addr;
	int data_size = data->blocksize * data->blocks;
	unsigned long data_phy_addr;
	void *dma_order_addr;

	if (data->flags == MMC_DATA_READ) {
		data_phy_addr = (unsigned long)data->dest;
	} else {
		data_phy_addr = (unsigned long)data->src;
	}

	if (desc == NULL || !((unsigned long long)desc & UNCACHED_MEMORY_ADDR) || !host->wdma_order_addr || !host->rdma_order_addr) {
		pr_debug("Invalid DMA address.\n");
		return -EINVAL;
	}

	
	desc->order_addr_low	= 0x0;
	desc->saddr_low		= data_phy_addr;
	desc->daddr		= host->ioaddr + SDIWRDAT;
	desc->length		= data_size / 4;
	desc->step_length	= 0x1;
	desc->step_times	= 0x1;

	pr_debug("data_phy_addr =  0x%x,data->flags=%d\n",data_phy_addr,data->flags);
	pr_debug("desc->order_addr_low = 0x%x\n",desc->order_addr_low);
	pr_debug("desc->saddr_low = 0x%x\n",desc->saddr_low);
	pr_debug("desc->daddr = 0x%x\n",desc->daddr);
	pr_debug("desc->length = 0x%x\n",desc->length);

	if (data->flags == MMC_DATA_READ) {
		desc->cmd	= DMA_CMD_INTMSK;
		dma_order_addr	= host->rdma_order_addr;
	} else {
		desc->cmd	= DMA_CMD_INTMSK | DMA_CMD_WRITE;
		dma_order_addr	= host->wdma_order_addr;
	}

	iowrite64((unsigned long)host->dma_desc_addr | DMA_OREDER_START, dma_order_addr);

	return 0;
}

static int sdio_transfer_data(struct sdio_host *host, struct mmc_data *data)
{
	int data_fin     = 0;
	int sdiintmsk    = 0;
	unsigned int err = 0;
	int timeout = 20000000;

	while (data_fin == 0 && timeout--) {
		sdiintmsk = sdio_readl(host, SDIINTMSK);
		data_fin  = SDIO_GET_DATAINT(sdiintmsk);
	}

	if (timeout < 0) {
		pr_debug("wait sdio data irq timeout.\n");
		return -ETIMEDOUT;
	}

	if (sdiintmsk & SDIO_INTMSK_CRCSTA) {
		pr_debug("DATA crc state error.\n");
		err = -ETIMEDOUT;
	} else if (sdiintmsk & SDIO_INTMSK_DATCRC) {
		pr_debug("DATA CRC error.\n");
		err = -1;
	} else if (sdiintmsk & SDIO_INTMSK_PROGERR) {
		pr_debug("DATA program error.\n");
		err = -1;
	} else if (sdiintmsk & SDIO_INTMSK_DATTOUT) {
		pr_debug("DATA timeout.\n");
		err = -ETIMEDOUT;
	}

	sdio_writel(host, SDIINTMSK, sdiintmsk & 0x1e);

	return err;
}

static unsigned int sdio_cmd_prepare_flag (struct mmc_cmd* cmd)
{
	unsigned int   flag = 0;
	unsigned short cmd_index;
	unsigned int   rsp;

	cmd_index = cmd->cmdidx;
	rsp       = cmd->resp_type;

	if (rsp == MMC_RSP_NONE) {
		return flag;
	}

	if (rsp & MMC_RSP_PRESENT) {
		flag |= SDIO_CMDCON_WAITRSP;
	}
// if response type is R2 and the crc bit set on, some unknow error will happen
	if (rsp & MMC_RSP_CRC) {
		flag |= SDIO_CMDCON_CHECKCRC;
	}

	if (rsp & MMC_RSP_136) {
		flag |= SDIO_CMDCON_LONGRSP;
	}

	if (cmd_index == MMC_CMD_READ_MULTIPLE_BLOCK || cmd_index == MMC_CMD_WRITE_MULTIPLE_BLOCK) {
		flag |= SDIO_CMDCON_AUTOSTOP;
	}

	return flag;
}

static int sdio_send_cmd(struct udevice *dev, struct mmc_cmd *cmd, struct mmc_data *data)
{
    struct sdio_host *host = dev_get_priv(dev);
    struct mmc *mmc = mmc_get_mmc_dev(dev);
	unsigned int   sdiintmsk;
	unsigned int   flag = 0;
	unsigned short cmd_index;
	unsigned int   cmd_fin = 0;
	int timeout = 20000;
	int err = 0;

	sdio_writel(host, SDIINTEN, 0x3ff);
	cmd_index = cmd->cmdidx;
	flag = sdio_cmd_prepare_flag(cmd);
	pr_debug("cmd_index = %d,cmd_arg = 0x%x\n",cmd->cmdidx,cmd->cmdarg);
	if(cmd_index == MMC_CMD_STOP_TRANSMISSION)
		return 0;

	if (data) {
		if (data->blocks > 4096 || data->blocks == 0) {
			pr_debug("DATA block argument is invalid\n");
			return -EINVAL;
		}

		unsigned int sdidatcon = SDIO_DATCON_DTST | SDIO_DATCON_DMAEN | data->blocks;

		if (mmc->bus_width == 4 || mmc->selected_mode & MMC_MODE_4BIT) {
			sdidatcon |= SDIO_DATCON_WIDEMODE;
		} else if (mmc->bus_width == 8 || mmc->selected_mode & MMC_MODE_8BIT)
			sdidatcon |= SDIO_DATCON_WIDEMODE8B;


		sdio_writel(host, SDIBSIZE, data->blocksize);
		sdio_writel(host, SDIDTIMER, 0x7fffff);
		sdio_writel(host, SDIDATCON, sdidatcon);


		err = sdio_prepare_dma(host, data);
		if (err) {
			return err;
		}
	}


	sdio_writel(host, SDICMDARG, cmd->cmdarg);
	sdio_writel(host, SDICMDCON, SDIO_MAKE_CMD(cmd_index, flag));

	while ((cmd_fin == 0) && timeout--) {
		sdiintmsk = sdio_readl(host, SDIINTMSK);
		cmd_fin = SDIO_GET_CMDINT(sdiintmsk);
		if (timeout == 0) {
			pr_debug("CMD check timeout!\n");
			err = -ETIMEDOUT;
			goto out;
		}
	}

	if (sdiintmsk & SDIO_INTMSK_RSPCRC) {
		pr_debug("CMD %d CRC error.\n", cmd_index);
		err = -1;
	} else if (sdiintmsk & SDIO_INTMSK_CMDTOUT) {
		pr_debug("CMD %d TIMEOUT error.\n", cmd_index);
		err = -ETIMEDOUT;
	} else if (sdiintmsk & SDIO_INTMSK_CMDFIN) {
		int i;
		if (flag & SDIO_CMDCON_LONGRSP) {
			for (i = 0; i < 4; i++) {
				cmd->response[i] = sdio_readl(host, SDIRSP0 + i*4);
			}
		} else {
			cmd->response[0] = sdio_readl(host, SDIRSP0);
		}
	} else {
		pr_debug("CMD %d NOT finished, INT_MSK reg is 0x%x\n", cmd_index, sdiintmsk);
		err = -1;
	}

	if (data) {
		err = sdio_transfer_data(host, data);
	}

out:
	sdio_writel(host, SDIINTMSK, (sdiintmsk & 0x1fe));

	sdio_writel(host, SDIINTEN, 0x0);
	return err;
}

static void sdio_set_clk(struct sdio_host *host, unsigned int clk)
{
	unsigned int clk_pre;

	if (clk == 0) {
		sdio_writel(host, SDICON, SDIO_CON_DIACLK);
		return;
	}
	
	clk_pre = DIV_ROUND_UP(host->clock,clk);
	if(clk_pre > 0xff || clk_pre == 0x0) 
		clk_pre = 0xff;
	sdio_writel(host, SDICON, SDIO_CON_ENCLK);
	sdio_writel(host, SDIPRE, clk_pre | SDIO_PRE_REVCLOCK);
}

static int sdio_set_ios(struct udevice *dev)
{
    struct sdio_host *host = dev_get_priv(dev);
    struct mmc *mmc = mmc_get_mmc_dev(dev);
	if (mmc->clock != host->clock) {
		sdio_set_clk(host, mmc->clock);
	}

	if (mmc->clk_disable) {
		sdio_set_clk(host, 0);
	}
	return 0;
}

static int sdio_mmc_get_cd(struct udevice *dev)
{
    return 1;
}

static const struct dm_mmc_ops sdio_mmc_ops = {
    .send_cmd = sdio_send_cmd,
    .set_ios = sdio_set_ios,
    .get_cd = sdio_mmc_get_cd,
};

static int sdio_mmc_probe(struct udevice *dev)
{
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
    struct sdio_mmc_plat *plat = dev_get_plat(dev);
    struct sdio_host *host = dev_get_priv(dev);
	
	
    plat->mmc.priv = host;
	upriv->mmc = &plat->mmc;
    
    return mmc_init(&plat->mmc);
}

static int sdio_mmc_bind(struct udevice *dev)
{
    struct sdio_mmc_plat *plat = dev_get_plat(dev);
    return mmc_bind(dev, &plat->mmc, &plat->cfg);
}

static int sdio_mmc_of_to_plat(struct udevice *dev)
{
    struct sdio_mmc_plat *plat = dev_get_plat(dev);
    struct sdio_host *host = dev_get_priv(dev);
	struct mmc_config *cfg;
	struct clk clk;
	int err;
	
	err = clk_get_by_index(dev, 0, &clk);
	if (!err) {
		err = clk_get_rate(&clk);
		if (!IS_ERR_VALUE(err))
			host->clock = err;
	} else if (err != -ENOENT && err != -ENODEV && err != -ENOSYS) {
		pr_debug("mmc failed to get clock\n");
		return err;
	}

	if (!host->clock)
		host->clock = dev_read_u32_default(dev, "clock-frequency",
						   CONFIG_SYS_SDIO_CLK);
	if (!host->clock)
		host->clock = CONFIG_SYS_SDIO_CLK;
	if (!host->clock) {
		pr_debug("mmc clock not defined\n");
		return -EINVAL;
	}

	host->ioaddr = PHYS_TO_UNCACHED((phys_addr_t)dev_read_addr(dev));
	host->dma_desc_addr = (unsigned int *)SDIO_DMA_DESCADDR;
	host->wdma_order_addr = (unsigned int *)(host->ioaddr + 0x400);
	host->rdma_order_addr = (unsigned int *)(host->ioaddr + 0x800);
    cfg = &plat->cfg;
    cfg->name = dev->name;
	cfg->host_caps = MMC_MODE_HS_52MHz | MMC_MODE_HS | MMC_MODE_4BIT | MMC_MODE_8BIT;
	cfg->voltages = MMC_VDD_32_33|MMC_VDD_33_34;
	cfg->f_min = host->clock / 256;
	cfg->f_max = host->clock;
	cfg->b_max = 1024;

	return 0;
}
static const struct udevice_id sdio_mmc_match[] = {
    { .compatible = "loongson,ls-mmc" },
    { }
};

U_BOOT_DRIVER(ls_mmc) = {
    .name = "ls_mmc",
    .id = UCLASS_MMC,
    .of_match = sdio_mmc_match,
    .bind = sdio_mmc_bind,
	.of_to_plat	= sdio_mmc_of_to_plat,
    .probe = sdio_mmc_probe,
    .ops = &sdio_mmc_ops,
    .priv_auto = sizeof(struct sdio_host),
    .plat_auto = sizeof(struct sdio_mmc_plat),
};

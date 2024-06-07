// SPDX-License-Identifier: GPL-2.0+
/*
 * Simulate an I2C port
 *
 * Copyright (c) 2024 loongson, Inc
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <i2c.h>
#include <log.h>
#include <linux/iopoll.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <dm/acpi.h>
#include <dm/lists.h>
#include <dm/device-internal.h>

/* Loongson I2C offset registers */
#define I2C_CR1			0x00
#define I2C_CR2			0x04
#define I2C_DR			0x10
#define I2C_SR1			0x14
#define I2C_SR2			0x18
#define I2C_CCR			0x1C
#define I2C_TRISE		0x20
#define I2C_FLTR		0x24

/* Loongson I2C control 1*/
#define I2C_CR1_POS		BIT(11)
#define I2C_CR1_ACK		BIT(10)
#define I2C_CR1_STOP	BIT(9)
#define I2C_CR1_START	BIT(8)
#define I2C_CR1_PE		BIT(0)

/* Loongson I2C control 2 */
#define I2C_CR2_FREQ_MASK	GENMASK(5, 0)
#define I2C_CR2_FREQ(n)		((n) & I2C_CR2_FREQ_MASK)
#define I2C_CR2_ITBUFEN		BIT(10)
#define I2C_CR2_ITEVTEN		BIT(9)
#define I2C_CR2_ITERREN		BIT(8)
#define I2C_CR2_IRQ_MASK	(I2C_CR2_ITBUFEN | \
				 I2C_CR2_ITEVTEN | \
				 I2C_CR2_ITERREN)

/* Loongson I2C Status 1 */
#define I2C_SR1_AF		BIT(10)
#define I2C_SR1_ARLO	BIT(9)
#define I2C_SR1_BERR	BIT(8)
#define I2C_SR1_TXE		BIT(7)
#define I2C_SR1_RXNE	BIT(6)
#define I2C_SR1_BTF		BIT(2)
#define I2C_SR1_ADDR	BIT(1)
#define I2C_SR1_SB		BIT(0)
#define I2C_SR1_ITEVTEN_MASK	(I2C_SR1_BTF | \
				 I2C_SR1_ADDR | \
				 I2C_SR1_SB)
#define I2C_SR1_ITBUFEN_MASK	(I2C_SR1_TXE | \
				 I2C_SR1_RXNE)
#define I2C_SR1_ITERREN_MASK	(I2C_SR1_AF | \
				 I2C_SR1_ARLO | \
				 I2C_SR1_BERR)

/* Loongson I2C Status 2 */
#define I2C_SR2_BUSY		BIT(1)

/* Loongson I2C Control Clock */
#define I2C_CCR_FS			BIT(15)
#define I2C_CCR_DUTY		BIT(14)

enum ls_i2c_speed {
	LS_I2C_SPEED_STANDARD,	/* 100 kHz */
	LS_I2C_SPEED_FAST,	/* 400 kHz */
};

/**
 * struct priv_msg - client specific data
 * @addr: 8-bit slave addr, including r/w bit
 * @count: number of bytes to be transferred
 * @buf: data buffer
 * @stop: last I2C msg to be sent, i.e. STOP to be generated
 * @result: result of the transfer
 */
struct priv_msg {
	u8 addr;
	u32 count;
	u8 *buf;
	bool stop;
	int result;
};

/**
 * struct ls_i2c_dev - private data of the controller
 * @adap: I2C adapter for this controller
 * @dev: device for this controller
 * @base: virtual memory area
 * @complete: completion of I2C message
 * @speed: Standard or Fast are supported
 * @msg: I2C transfer information
 */
struct ls_i2c_dev {
	struct device *dev;
	void __iomem *base;
	int speed;
	struct priv_msg msg;
};
static inline void i2c_set_bits(void __iomem *reg, u32 mask)
{
	writel(readl(reg) | mask, reg);
}

static inline void i2c_clr_bits(void __iomem *reg, u32 mask)
{
	writel(readl(reg) & ~mask, reg);
}
static void ls_i2c_write_byte(struct ls_i2c_dev *i2c_dev, u8 byte)
{
	writel(byte, i2c_dev->base + I2C_DR);
}

static void ls_i2c_read_msg(struct ls_i2c_dev *i2c_dev)
{
	struct priv_msg *msg = &i2c_dev->msg;

	*msg->buf++ = readl(i2c_dev->base + I2C_DR);
	msg->count--;
}

static void ls_i2c_terminate_xfer(struct ls_i2c_dev *i2c_dev)
{
	struct priv_msg *msg = &i2c_dev->msg;

	i2c_set_bits(i2c_dev->base + I2C_CR1, (1 << (8 + msg->stop)));
}

static void ls_i2c_handle_write(struct ls_i2c_dev *i2c_dev)
{
	struct priv_msg *msg = &i2c_dev->msg;

	udelay(150);

	if (msg->count) {
		ls_i2c_write_byte(i2c_dev, *msg->buf++);
		msg->count--;
		if (!msg->count) {
			ls_i2c_terminate_xfer(i2c_dev);
		}
	}
}

static void ls_i2c_handle_read(struct ls_i2c_dev *i2c_dev, int flag)
{
	struct priv_msg *msg = &i2c_dev->msg;
	void __iomem *reg = i2c_dev->base + I2C_CR1;
	int i;

	udelay(150); //When SR1 is set up, the data is not immediately ready

	switch (msg->count) {
	case 1:
		/* only transmit 1 bytes condition */
		ls_i2c_read_msg(i2c_dev);
		break;
	case 2:
		if (flag != 1) {
			/* ensure only transmit 2 bytes condition */
			break;
		}
		i2c_set_bits(i2c_dev->base + I2C_CR1, (1 << (8 + msg->stop)));


		for (i = 2; i > 0; i--)
			ls_i2c_read_msg(i2c_dev);

		i2c_clr_bits(i2c_dev->base + I2C_CR1, I2C_CR1_POS);
		break;
	case 3:
		if (readl(i2c_dev->base + I2C_CR2) & I2C_CR2_ITBUFEN) {
			break;
		}
		i2c_clr_bits(reg, I2C_CR1_ACK);
	default:
		ls_i2c_read_msg(i2c_dev);
	}
}

static inline u8 i2c_8bit_addr_from_msg(const struct i2c_msg *msg)
{
    return (msg->addr << 1) | (msg->flags & I2C_M_RD ? 1 : 0);
}


#define INPUT_REF_CLK 120000000

static int ls_i2c_hw_config(struct ls_i2c_dev *i2c_dev)
{
	u32 val;
	u32 ccr = 0;

	/* reference clock determination the cnfigure val(0x3f) */
	i2c_set_bits(i2c_dev->base + I2C_CR2, 0x3f);
	i2c_set_bits(i2c_dev->base + I2C_TRISE, 0x3f);

	if (i2c_dev->speed == LS_I2C_SPEED_STANDARD) {
		val = DIV_ROUND_UP(INPUT_REF_CLK, 100000 * 2);
	} else {
		val = DIV_ROUND_UP(INPUT_REF_CLK, 400000 * 3);

		/* Select Fast mode */
		ccr |= I2C_CCR_FS;
	}
	ccr |= val & 0xfff;
	writel(ccr, i2c_dev->base + I2C_CCR);

	/* Enable I2C */
	writel(I2C_CR1_PE, i2c_dev->base + I2C_CR1);

	return 0;
}
static int ls_i2c_wait_free_bus(struct ls_i2c_dev *i2c_dev)
{
	u32 status;
	int ret;
	ret = readl_poll_timeout(i2c_dev->base + I2C_SR2, status, !(status & I2C_SR2_BUSY),  1000);
	if (ret) {
		ret = -EBUSY;
	}

	return ret;
}

/**
 * ls_i2c_handle_rx_addr()
 * master receiver
 * @i2c_dev: Controller's private data
 */
static void ls_i2c_handle_rx_addr(struct ls_i2c_dev *i2c_dev)
{
	struct priv_msg *msg = &i2c_dev->msg;

	switch (msg->count) {
	case 0:
		ls_i2c_terminate_xfer(i2c_dev);
		break;
	case 1:
		i2c_clr_bits(i2c_dev->base + I2C_CR1,
				(I2C_CR1_ACK | I2C_CR1_POS));
		/* start or stop */
		i2c_set_bits(i2c_dev->base + I2C_CR1, (1 << (8 + msg->stop)));
		break;
	case 2:
		i2c_clr_bits(i2c_dev->base + I2C_CR1, I2C_CR1_ACK);
		i2c_set_bits(i2c_dev->base + I2C_CR1, I2C_CR1_POS);
		break;

	default:
		i2c_set_bits(i2c_dev->base + I2C_CR1, I2C_CR1_ACK);
		i2c_clr_bits(i2c_dev->base + I2C_CR1, I2C_CR1_POS);
		break;
	}
}

static u32 ls_i2c_isr_error(u32 status, void *data)
{
	struct ls_i2c_dev *i2c_dev = data;
	struct priv_msg *msg = &i2c_dev->msg;

	/* Arbitration lost */
	if (status & I2C_SR1_ARLO) {
		i2c_clr_bits(i2c_dev->base + I2C_SR1, I2C_SR1_ARLO);
		msg->result = -EAGAIN;
	}

	/*
	 * Acknowledge failure:
	 * In master transmitter mode a Stop must be generated by software
	 */
	if (status & I2C_SR1_AF) {
		i2c_set_bits(i2c_dev->base + I2C_CR1, I2C_CR1_STOP);
		i2c_clr_bits(i2c_dev->base + I2C_SR1, I2C_SR1_AF);
		msg->result = -ENODEV;
	}

	/* Bus error */
	if (status & I2C_SR1_BERR) {
		i2c_clr_bits(i2c_dev->base + I2C_SR1, I2C_SR1_BERR);
		msg->result = -EIO;
	}


	return msg->result;
}
static int ls_i2c_wait_flags(struct ls_i2c_dev *i2c_dev,u32 flags, u32 *status)
{
	static volatile u32 timeout = 1000000;
	*status = readl(i2c_dev->base + I2C_SR1);
	while(!(*status & flags)) {
			if(!timeout) 
				return -ETIMEDOUT;
 	    *status = readl(i2c_dev->base + I2C_SR1);
		timeout--;
		}

	return 0;
}
static int ls_i2c_xfer_msg(struct ls_i2c_dev *i2c_dev,struct i2c_msg *msg, bool is_stop)
{
	struct priv_msg *priv_msg = &i2c_dev->msg;
	int ret, i;
	u32 status, event, mask;
	priv_msg->addr   = i2c_8bit_addr_from_msg(msg);
	priv_msg->buf    = msg->buf;
	priv_msg->count  = msg->len;
	priv_msg->stop   = is_stop;
	priv_msg->result = 0;
	mask = I2C_SR1_SB; 
	ls_i2c_wait_flags(i2c_dev, mask, &status);

		ls_i2c_write_byte(i2c_dev, priv_msg->addr);
	mask = I2C_SR1_TXE | I2C_SR1_RXNE | I2C_SR1_BTF | I2C_SR1_ADDR | I2C_SR1_ITERREN_MASK;
		/*start*/
	do {
			ret = ls_i2c_wait_flags(i2c_dev, mask, &status);
			if (ret)
				printf("wait timeout\n:");
			if(status & (I2C_SR1_ARLO | I2C_SR1_AF | I2C_SR1_BERR)) {
				return ls_i2c_isr_error(status, i2c_dev);
			}
			event = status;

		/* I2C Address sent */
			if (event & I2C_SR1_ADDR) {
				if (priv_msg->addr & I2C_M_RD)
					ls_i2c_handle_rx_addr(i2c_dev);
				if (priv_msg->count == 0)
					ls_i2c_terminate_xfer(i2c_dev);
	
				/* Clear ADDR flag */
				readl(i2c_dev->base + I2C_SR2);
			}
	
		if (priv_msg->addr & I2C_M_RD) {
			/* RX */
			if (event & 0x40)
				ls_i2c_handle_read(i2c_dev, 0);
			if (event & 0x4)
				ls_i2c_handle_read(i2c_dev, 1);
		} else {
			/* TX */
			if (event & 0x84)
				for (i = 0; i < 1 + !!((event & 0x84) == 0x84); i++)
					ls_i2c_handle_write(i2c_dev);
		}
		
	} while (priv_msg->count);

	ret = priv_msg->result;

	return 0;
}

static int lsfs_i2c_xfer(struct udevice *bus, struct i2c_msg *msg, int num)
{
	struct ls_i2c_dev *i2c_dev = dev_get_priv(bus);
	int ret, i;
	ret = ls_i2c_wait_free_bus(i2c_dev);
	if (ret)
		return ret;

	/* START generation */
	i2c_set_bits(i2c_dev->base + I2C_CR1, I2C_CR1_START);

	for (i = 0; i < num && !ret; i++)
		ret = ls_i2c_xfer_msg(i2c_dev, &msg[i], i == num - 1);

	return (ret < 0) ? ret : 0;
}

static int lsfs_i2c_probe(struct udevice *bus)
{
	struct ls_i2c_dev *i2c_dev = dev_get_priv(bus);

	i2c_dev->base =  PHYS_TO_UNCACHED(dev_read_addr_ptr(bus));
	i2c_dev->speed = LS_I2C_SPEED_STANDARD;
	ls_i2c_hw_config(i2c_dev);
	return 0;
}

static const struct dm_i2c_ops lsfs_i2c_ops = {
	.xfer		= lsfs_i2c_xfer,
};

static const struct udevice_id lsfs_i2c_ids[] = {
	{ .compatible = "loongson,lsfs-i2c" },
	{ }
};

U_BOOT_DRIVER(lsfs_i2c) = {
	.name	= "lsfs_i2c",
	.id	= UCLASS_I2C,
	.of_match = lsfs_i2c_ids,
	.probe = lsfs_i2c_probe,
	.ops	= &lsfs_i2c_ops,
	.priv_auto	= sizeof(struct ls_i2c_dev),
};

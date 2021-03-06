/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/arch/mx35.h>
#include <asm/arch/mx35_pins.h>
#include <asm/arch/iomux.h>
#include <i2c.h>
#include <linux/types.h>

#ifdef CONFIG_CMD_MMC
#include <mmc.h>
#include <fsl_esdhc.h>
#endif

#ifdef CONFIG_ARCH_MMU
#include <asm/mmu.h>
#include <asm/arch/mmu.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

static u32 system_rev;

u32 get_board_rev(void)
{
	return system_rev;
}

static inline void setup_soc_rev(void)
{
	int reg;
	reg = __REG(IIM_BASE_ADDR + IIM_SREV);
	if (!reg) {
		reg = __REG(ROMPATCH_REV);
		reg <<= 4;
	} else
		reg += CHIP_REV_1_0;
	system_rev = 0x35000 + (reg & 0xFF);
}

static inline void set_board_rev(int rev)
{
	system_rev =  (system_rev & ~(0xF << 8)) | (rev & 0xF) << 8;
}

int is_soc_rev(int rev)
{
	return (system_rev & 0xFF) - rev;
}

#ifdef CONFIG_ARCH_MMU
void board_mmu_init(void)
{
	unsigned long ttb_base = PHYS_SDRAM_1 + 0x40000;
	unsigned long i;

	/*
	 * Set the TTB register
	 */
	asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb_base) /*:*/);

	/*
	 * Set the Domain Access Control Register
	 */
	i = ARM_ACCESS_DACR_DEFAULT;
	asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(i) /*:*/);

	/*
	 * First clear all TT entries - ie Set them to Faulting
	 */
	memset((void *)ttb_base, 0, ARM_FIRST_LEVEL_PAGE_TABLE_SIZE);
	/* Actual   Virtual  Size   Attributes   	Function */
	/* Base     Base     MB     cached? buffered?  access permissions */
	/* xxx00000 xxx00000 */
	X_ARM_MMU_SECTION(0x000, 0xF00, 0x1,
			  ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			  ARM_ACCESS_PERM_RW_RW); /* ROM */
	X_ARM_MMU_SECTION(0x100, 0x100, 0x1,
			  ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			  ARM_ACCESS_PERM_RW_RW); /* iRAM */
	X_ARM_MMU_SECTION(0x300, 0x300, 0x1,
			  ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			  ARM_ACCESS_PERM_RW_RW); /* L2CC */
	/* Internal Regsisters upto SDRAM*/
	X_ARM_MMU_SECTION(0x400, 0x400, 0x400,
			  ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			  ARM_ACCESS_PERM_RW_RW);
	X_ARM_MMU_SECTION(0x800, 0x000, 0x80,
			  ARM_CACHEABLE, ARM_BUFFERABLE,
			  ARM_ACCESS_PERM_RW_RW); /* SDRAM 0:128M*/
	X_ARM_MMU_SECTION(0x800, 0x800, 0x80,
			  ARM_CACHEABLE, ARM_BUFFERABLE,
			  ARM_ACCESS_PERM_RW_RW); /* SDRAM 0:128M*/
	X_ARM_MMU_SECTION(0x800, 0x880, 0x80,
			  ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			  ARM_ACCESS_PERM_RW_RW); /* SDRAM 0:128M*/
	X_ARM_MMU_SECTION(0x900, 0x900, 0x80,
			  ARM_CACHEABLE, ARM_BUFFERABLE,
			  ARM_ACCESS_PERM_RW_RW); /* SDRAM 1:128M*/
	X_ARM_MMU_SECTION(0xA00, 0xA00, 0x40,
			  ARM_CACHEABLE, ARM_BUFFERABLE,
			  ARM_ACCESS_PERM_RW_RW); /* Flash */
	X_ARM_MMU_SECTION(0xB00, 0xB00, 0x20,
			  ARM_CACHEABLE, ARM_BUFFERABLE,
			  ARM_ACCESS_PERM_RW_RW); /* PSRAM */
	/* ESDCTL, WEIM, M3IF, EMI, NFC, External I/O */
	X_ARM_MMU_SECTION(0xB20, 0xB20, 0x1E0,
			  ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			  ARM_ACCESS_PERM_RW_RW);
}
#endif

int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}

int board_init(void)
{
	int pad;

	setup_soc_rev();

	/* enable clocks */
	__REG(CCM_BASE_ADDR + CLKCTL_CGR0) |= 0x003F0000;
	__REG(CCM_BASE_ADDR + CLKCTL_CGR1) |= 0x00030FFF;

	// IO config 2013.11.5
	// LED1_TEST
	mxc_request_iomux(MX35_PIN_STXD5, MUX_CONFIG_GPIO);  
	__REG(GPIO1_BASE_ADDR + 0x04) |= 1 << 0;       // set  output
	__REG(GPIO1_BASE_ADDR + 0x00) |= (1 << 0);       // set  high

	// LED2_TEST
	mxc_request_iomux(MX35_PIN_SRXD5, MUX_CONFIG_GPIO);  
	__REG(GPIO1_BASE_ADDR + 0x04) |= 1 << 1;       // set  output
	__REG(GPIO1_BASE_ADDR + 0x00) &= ~(1 << 1);       // set  high

	// LED flash light (module 1)
	// LED_BOOST
	mxc_request_iomux(MX35_PIN_SCK5, MUX_CONFIG_GPIO);  
	__REG(GPIO1_BASE_ADDR + 0x04) |= 1 << 2;       // set  output
	__REG(GPIO1_BASE_ADDR + 0x00) |= 1 << 2;       // set  high
	// LED_HIGH_BOOST#
	mxc_request_iomux(MX35_PIN_STXFS4, MUX_CONFIG_GPIO);  
	__REG(GPIO2_BASE_ADDR + 0x04) |= 1 << 31;       // set  output
	__REG(GPIO2_BASE_ADDR + 0x00) |= 1 << 31;       // set  high	

	// LED flash light (module 2)
	// LED_TORCH
	mxc_request_iomux(MX35_PIN_SCK4, MUX_CONFIG_GPIO);  
	__REG(GPIO2_BASE_ADDR + 0x04) |= 1 << 30;       // set  output
	__REG(GPIO2_BASE_ADDR + 0x00) &= (~(1 << 30));       // set  low
	// LED_MODE
	mxc_request_iomux(MX35_PIN_STXFS5, MUX_CONFIG_GPIO); 
	__REG(GPIO1_BASE_ADDR + 0x04) |= 1 << 3;       // set  output
	__REG(GPIO1_BASE_ADDR + 0x00) &= (~(1 << 3));       // set  low

	// config LAN8710 reset 
	// PHY_RESET#
	mxc_request_iomux(MX35_PIN_GPIO3_0, MUX_CONFIG_FUNC); 
	__REG(GPIO3_BASE_ADDR + 0x04) |= 1 << 0;       // set  output
	__REG(GPIO3_BASE_ADDR + 0x00) |= 1 << 0;       // set  high

	// config camera sensor STANDBY
	// SENSOR_STANDBY
	mxc_request_iomux(MX35_PIN_ATA_DATA9, MUX_CONFIG_GPIO); 
	__REG(GPIO2_BASE_ADDR + 0x04) |= 1 << 22;       // set  output
	__REG(GPIO2_BASE_ADDR + 0x00) |= 1 << 22;       // set  high standby

	// SENSOR_EXPOSURE
	mxc_request_iomux(MX35_PIN_ATA_DATA10, MUX_CONFIG_GPIO); 
	__REG(GPIO2_BASE_ADDR + 0x04) |= 1 << 23;       // set  output
	__REG(GPIO2_BASE_ADDR + 0x00) &= (~(1 << 23));       // set  high standby
	
	
	/* setup pins for I2C1 */
	mxc_request_iomux(MX35_PIN_I2C1_CLK, MUX_CONFIG_SION);
	mxc_request_iomux(MX35_PIN_I2C1_DAT, MUX_CONFIG_SION);

	pad = (PAD_CTL_HYS_SCHMITZ | PAD_CTL_PKE_ENABLE \
			| PAD_CTL_PUE_PUD | PAD_CTL_ODE_OpenDrain);

	mxc_iomux_set_pad(MX35_PIN_I2C1_CLK, pad);
	mxc_iomux_set_pad(MX35_PIN_I2C1_DAT, pad);

	/* setup pins for FEC */
	mxc_request_iomux(MX35_PIN_FEC_TX_CLK, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_RX_CLK, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_RX_DV, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_COL, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_RDATA0, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_TDATA0, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_TX_EN, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_MDC, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_MDIO, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_TX_ERR, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_RX_ERR, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_CRS, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_RDATA1, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_TDATA1, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_RDATA2, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_TDATA2, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_RDATA3, MUX_CONFIG_FUNC);
	mxc_request_iomux(MX35_PIN_FEC_TDATA3, MUX_CONFIG_FUNC);

	pad = (PAD_CTL_DRV_3_3V | PAD_CTL_PUE_PUD | PAD_CTL_ODE_CMOS | \
			PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW);

	mxc_iomux_set_pad(MX35_PIN_FEC_TX_CLK, pad | PAD_CTL_HYS_SCHMITZ | \
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_RX_CLK, pad | PAD_CTL_HYS_SCHMITZ | \
			PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_RX_DV, pad | PAD_CTL_HYS_SCHMITZ | \
			 PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_COL, pad | PAD_CTL_HYS_SCHMITZ | \
			  PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_RDATA0, pad | PAD_CTL_HYS_SCHMITZ | \
			  PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_TDATA0, pad | PAD_CTL_HYS_CMOS | \
			  PAD_CTL_PKE_NONE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_TX_EN, pad | PAD_CTL_HYS_CMOS | \
			  PAD_CTL_PKE_NONE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_MDC, pad | PAD_CTL_HYS_CMOS | \
			  PAD_CTL_PKE_NONE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_MDIO, pad | PAD_CTL_HYS_SCHMITZ | \
			  PAD_CTL_PKE_ENABLE | PAD_CTL_22K_PU);
	mxc_iomux_set_pad(MX35_PIN_FEC_TX_ERR, pad | PAD_CTL_HYS_CMOS | \
			  PAD_CTL_PKE_NONE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_RX_ERR, pad | PAD_CTL_HYS_SCHMITZ | \
			  PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_CRS, pad | PAD_CTL_HYS_SCHMITZ | \
			  PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_RDATA1, pad | PAD_CTL_HYS_SCHMITZ | \
			  PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_TDATA1, pad | PAD_CTL_HYS_CMOS | \
			  PAD_CTL_PKE_NONE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_RDATA2, pad | PAD_CTL_HYS_SCHMITZ | \
			  PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_TDATA2, pad | PAD_CTL_HYS_CMOS | \
			  PAD_CTL_PKE_NONE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_RDATA3, pad | PAD_CTL_HYS_SCHMITZ | \
			  PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PD);
	mxc_iomux_set_pad(MX35_PIN_FEC_TDATA3, pad | PAD_CTL_HYS_CMOS | \
			  PAD_CTL_PKE_NONE | PAD_CTL_100K_PD);

	gd->bd->bi_arch_number = MACH_TYPE_MX35_3DS;	/* board id for linux */
	gd->bd->bi_boot_params = 0x80000100;	/* address of boot parameters */

	return 0;
}

#ifdef BOARD_LATE_INIT
static inline int board_detect(void)
{
	u8 buf[4];
	int id;

	if (i2c_read(0x08, 0x7, 1, buf, 3) < 0) {
		printf("board_late_init: read PMIC@0x08:0x7 fail\n");
		return 0;
	}
	id = (buf[0] << 16) + (buf[1] << 8) + buf[2];
	printf("PMIC@0x08:0x7 is %x\n", id);
	id = (id >> 6) & 0x7;
	if (id == 0x7) {
		set_board_rev(1);
		return 1;
	}
	set_board_rev(0);
	return 0;
}

int board_late_init(void)
{
	u8 reg[3];
	int i;

	if (board_detect()) {
		mxc_request_iomux(MX35_PIN_WATCHDOG_RST, MUX_CONFIG_SION |
					MUX_CONFIG_ALT1);
		printf("i.MX35 CPU board version 2.0\n");
		if (i2c_read(0x08, 0x1E, 1, reg, 3)) {
			printf("board_late_init: read PMIC@0x08:0x1E fail\n");
			return 0;
		}
		reg[2] |= 0x3;
		if (i2c_write(0x08, 0x1E, 1, reg, 3)) {
			printf("board_late_init: write PMIC@0x08:0x1E fail\n");
			return 0;
		}
		if (i2c_read(0x08, 0x20, 1, reg, 3)) {
			printf("board_late_init: read PMIC@0x08:0x20 fail\n");
			return 0;
		}
		reg[2] |= 0x1;
		if (i2c_write(0x08, 0x20, 1, reg, 3)) {
			printf("board_late_init: write PMIC@0x08:0x20 fail\n");
			return 0;
		}
		mxc_request_iomux(MX35_PIN_COMPARE, MUX_CONFIG_GPIO);
		mxc_iomux_set_input(MUX_IN_GPIO1_IN_5, INPUT_CTL_PATH0);
		__REG(GPIO1_BASE_ADDR + 0x04) |= 1 << 5;
		__REG(GPIO1_BASE_ADDR) |= 1 << 5;
		
	} else
		printf("i.MX35 CPU board version 1.0\n");

	if (i2c_read(0x69, 0x20, 1, reg, 1) < 0) {
		printf("board_late_init: read PMIC@0x69:0x20 fail\n");
		return 0;
	}

	reg[0] |= 0x4;
	if (i2c_write(0x69, 0x20, 1, reg, 1) < 0) {
		printf("board_late_init: write back PMIC@0x69:0x20 fail\n");
		return 0;
	}

	for (i = 0; i < 1000; i++)
		udelay(200);

	if (i2c_read(0x69, 0x1A, 1, reg, 1) < 0) {
		printf("board_late_init: read PMIC@0x69:0x1A fail\n");
		return 0;
	}

	reg[0] &= 0x7F;
	if (i2c_write(0x69, 0x1A, 1, reg, 1) < 0) {
		printf("board_late_init: write back PMIC@0x69:0x1A fail\n");
		return 0;
	}
	for (i = 0; i < 1000; i++)
		udelay(200);

	reg[0] |= 0x80;
	if (i2c_write(0x69, 0x1A, 1, reg, 1) < 0) {
		printf("board_late_init: 2st write back PMIC@0x69:0x1A fail\n");
		return 0;
	}

	return 0;
}
#endif

int checkboard(void)
{
	printf("Board: iMX353 Rev00 ");

	if (system_rev & CHIP_REV_2_0)
		printf("2.0 [");
	else
		printf("1.0 [");

	switch (__REG(CCM_BASE_ADDR + CLKCTL_RCSR) & 0x0F) {
	case 0x0000:
		printf("POR");
		break;
	case 0x0002:
		printf("JTAG");
		break;
	case 0x0004:
		printf("RST");
		break;
	case 0x0008:
		printf("WDT");
		break;
	default:
		printf("unknown");
	}
	printf("]\n");
	return 0;
}

#if defined(CONFIG_SMC911X)
extern int smc911x_initialize(u8 dev_num, int base_addr);
#endif

int board_eth_init(bd_t *bis)
{
	int rc = -ENODEV;
#if defined(CONFIG_SMC911X)
	rc = smc911x_initialize(0, CONFIG_SMC911X_BASE);
#endif

	/* nick20131023_01 remove cpu_eth_init out of this function */
	//cpu_eth_init(bis);

	return rc;
}

#ifdef CONFIG_CMD_MMC

u32 *imx_esdhc_base_addr;

int esdhc_gpio_init(void)
{
	u32 interface_esdhc = 0;
	u32 pad_val = 0;

	interface_esdhc = (readl(IIM_BASE_ADDR + 0x80c)) & (0x000000C0) >> 6;

	/* IOMUX PROGRAMMING */
	switch (interface_esdhc) {
	case 0:
		imx_esdhc_base_addr = \
				(u32 *)MMC_SDHC1_BASE_ADDR;

		pad_val = PAD_CTL_PUE_PUD | PAD_CTL_PKE_ENABLE |
			PAD_CTL_HYS_SCHMITZ | PAD_CTL_DRV_HIGH |
			PAD_CTL_100K_PD | PAD_CTL_SRE_FAST;
		mxc_request_iomux(MX35_PIN_SD1_CMD,
			MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_iomux_set_pad(MX35_PIN_SD1_CMD, pad_val);

		pad_val = PAD_CTL_PUE_PUD | PAD_CTL_PKE_ENABLE |
				PAD_CTL_HYS_SCHMITZ | PAD_CTL_DRV_HIGH |
				PAD_CTL_100K_PU | PAD_CTL_SRE_FAST;
		mxc_request_iomux(MX35_PIN_SD1_CLK,
				MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_iomux_set_pad(MX35_PIN_SD1_CLK, pad_val);
		mxc_request_iomux(MX35_PIN_SD1_DATA0,
			  MUX_CONFIG_FUNC);
		mxc_iomux_set_pad(MX35_PIN_SD1_DATA0, pad_val);
		mxc_request_iomux(MX35_PIN_SD1_DATA3,
			  MUX_CONFIG_FUNC);
		mxc_iomux_set_pad(MX35_PIN_SD1_DATA3, pad_val);

		break;
	case 1:
		imx_esdhc_base_addr = \
				(u32 *)MMC_SDHC2_BASE_ADDR;

		mxc_request_iomux(MX35_PIN_SD2_CLK,
			  MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_request_iomux(MX35_PIN_SD2_CMD,
			  MUX_CONFIG_FUNC | MUX_CONFIG_SION);
		mxc_request_iomux(MX35_PIN_SD2_DATA0,
			  MUX_CONFIG_FUNC);
		mxc_request_iomux(MX35_PIN_SD2_DATA3,
			  MUX_CONFIG_FUNC);

		pad_val = PAD_CTL_PUE_PUD | PAD_CTL_PKE_ENABLE |
			PAD_CTL_HYS_SCHMITZ | PAD_CTL_DRV_MAX |
			PAD_CTL_100K_PD | PAD_CTL_SRE_FAST;
		mxc_iomux_set_pad(MX35_PIN_SD2_CMD, pad_val);

		pad_val = PAD_CTL_PUE_PUD | PAD_CTL_PKE_ENABLE |
				PAD_CTL_HYS_SCHMITZ | PAD_CTL_DRV_HIGH |
				PAD_CTL_100K_PU | PAD_CTL_SRE_FAST;
		mxc_iomux_set_pad(MX35_PIN_SD2_CLK, pad_val);
		mxc_iomux_set_pad(MX35_PIN_SD2_DATA0, pad_val);
		mxc_iomux_set_pad(MX35_PIN_SD2_DATA3, pad_val);

		break;
	case 2:
		imx_esdhc_base_addr = \
				(u32 *)MMC_SDHC3_BASE_ADDR;

		printf("TO2 ESDHC3 not supported!");
		break;
	default:
		break;
	}

	return 0;
}

int board_mmc_init(void)
{
	if (!esdhc_gpio_init())
		return fsl_esdhc_mmc_init(gd->bd);
	else
		return -1;
}
#endif

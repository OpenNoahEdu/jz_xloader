/*
  * Copyright (c) 2009, yajin <yajin@vm-kernel.org>
  * Copyright (c) 2005-2008  Ingenic Semiconductor Inc.
  * Author: <jlwei@ingenic.cn>
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License as
  * published by the Free Software Foundation; either version 2 of
  * the License, or (at your option) any later version.
  */



#include <jz4740.h>
#include <board.h>

#define VERSION "0.01"

static void gpio_init(void)
{
    __gpio_as_sdram_32bit();
    __gpio_as_uart0();
    __gpio_as_nand();
}

static void nand_enable()
{
    REG_EMC_NFCSR |= EMC_NFCSR_NFE1;
    REG_EMC_SMCR1 = 0x04444400;
}

/* PLL output clock = EXTAL * NF / (NR * NO)
 *
 * NF = FD + 2, NR = RD + 2
 * NO = 1 (if OD = 0), NO = 2 (if OD = 1 or 2), NO = 4 (if OD = 3)
 */
static void pll_init(void)
{
    register unsigned int cfcr, plcr1;
    int n2FR[33] = {
        0, 0, 1, 2, 3, 0, 4, 0, 5, 0, 0, 0, 6, 0, 0, 0,
        7, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0,
        9
    };
    int div[5] = { 0, 3, 3, 3, 3 };     /* divisors of I:S:P:M:L */
    int nf, pllout2;

    cfcr = CPM_CPCCR_CLKOEN |
        (n2FR[div[0]] << CPM_CPCCR_CDIV_BIT) |
        (n2FR[div[1]] << CPM_CPCCR_HDIV_BIT) |
        (n2FR[div[2]] << CPM_CPCCR_PDIV_BIT) |
        (n2FR[div[3]] << CPM_CPCCR_MDIV_BIT) |
        (n2FR[div[4]] << CPM_CPCCR_LDIV_BIT);

    pllout2 = (cfcr & CPM_CPCCR_PCS) ? CFG_CPU_SPEED : (CFG_CPU_SPEED / 2);

    /* Init USB Host clock, pllout2 must be n*48MHz */
    REG_CPM_UHCCDR = pllout2 / 48000000 - 1;

    nf = CFG_CPU_SPEED * 2 / CFG_EXTAL;
    plcr1 = ((nf + 2) << CPM_CPPCR_PLLM_BIT) |  /* FD */
        (0 << CPM_CPPCR_PLLN_BIT) |     /* RD=0, NR=2 */
        (0 << CPM_CPPCR_PLLOD_BIT) |    /* OD=0, NO=1 */
        (0x20 << CPM_CPPCR_PLLST_BIT) | /* PLL stable time */
        CPM_CPPCR_PLLEN;        /* enable PLL */

    /* init PLL */
    REG_CPM_CPCCR = cfcr;
    REG_CPM_CPPCR = plcr1;

}

/*
 * Init SDRAM memory.
 */

static void sdram_init(void)
{
    register unsigned int dmcr0, dmcr, sdmode, tmp, cpu_clk, mem_clk, ns;

    unsigned int cas_latency_sdmr[2] = {
        EMC_SDMR_CAS_2,
        EMC_SDMR_CAS_3,
    };

    unsigned int cas_latency_dmcr[2] = {
        1 << EMC_DMCR_TCL_BIT,  /* CAS latency is 2 */
        2 << EMC_DMCR_TCL_BIT   /* CAS latency is 3 */
    };

    int div[] = { 1, 2, 3, 4, 6, 8, 12, 16, 24, 32 };

    cpu_clk = CFG_CPU_SPEED;
    mem_clk = cpu_clk * div[__cpm_get_cdiv()] / div[__cpm_get_mdiv()];

    //REG_EMC_BCR = 0;      /* Disable bus release */
    REG_EMC_RTCSR = 0;          /* Disable clock for counting */
    REG_EMC_RTCOR = 0;
    REG_EMC_RTCNT = 0;

    /* Fault DMCR value for mode register setting */
#define SDRAM_ROW0    11
#define SDRAM_COL0     8
#define SDRAM_BANK40   0

    dmcr0 = ((SDRAM_ROW0 - 11) << EMC_DMCR_RA_BIT) |
        ((SDRAM_COL0 - 8) << EMC_DMCR_CA_BIT) |
        (SDRAM_BANK40 << EMC_DMCR_BA_BIT) |
        (CFG_SDRAM_BW16 << EMC_DMCR_BW_BIT) |
        EMC_DMCR_EPIN | cas_latency_dmcr[((CFG_SDRAM_CASL == 3) ? 1 : 0)];

    /* Basic DMCR value */
    dmcr = ((CFG_SDRAM_ROW - 11) << EMC_DMCR_RA_BIT) |
        ((CFG_SDRAM_COL - 8) << EMC_DMCR_CA_BIT) |
        (CFG_SDRAM_BANK4 << EMC_DMCR_BA_BIT) |
        (CFG_SDRAM_BW16 << EMC_DMCR_BW_BIT) |
        EMC_DMCR_EPIN | cas_latency_dmcr[((CFG_SDRAM_CASL == 3) ? 1 : 0)];

    /* SDRAM timimg */
    ns = 1000000000 / mem_clk;
    tmp = CFG_SDRAM_TRAS / ns;
    if (tmp < 4)
        tmp = 4;
    if (tmp > 11)
        tmp = 11;
    dmcr |= ((tmp - 4) << EMC_DMCR_TRAS_BIT);
    tmp = CFG_SDRAM_RCD / ns;
    if (tmp > 3)
        tmp = 3;
    dmcr |= (tmp << EMC_DMCR_RCD_BIT);
    tmp = CFG_SDRAM_TPC / ns;
    if (tmp > 7)
        tmp = 7;
    dmcr |= (tmp << EMC_DMCR_TPC_BIT);
    tmp = CFG_SDRAM_TRWL / ns;
    if (tmp > 3)
        tmp = 3;
    dmcr |= (tmp << EMC_DMCR_TRWL_BIT);
    tmp = (CFG_SDRAM_TRAS + CFG_SDRAM_TPC) / ns;
    if (tmp > 14)
        tmp = 14;
    dmcr |= (((tmp + 1) >> 1) << EMC_DMCR_TRC_BIT);

    /* SDRAM mode value */
    sdmode = EMC_SDMR_BT_SEQ |
        EMC_SDMR_OM_NORMAL |
        EMC_SDMR_BL_4 | cas_latency_sdmr[((CFG_SDRAM_CASL == 3) ? 1 : 0)];

    /* Stage 1. Precharge all banks by writing SDMR with DMCR.MRSET=0 */
    REG_EMC_DMCR = dmcr;
    REG8(EMC_SDMR0 | sdmode) = 0;

    /* Wait for precharge, > 200us */
    tmp = (cpu_clk / 1000000) * 1000;
    while (tmp--);

    /* Stage 2. Enable auto-refresh */
    REG_EMC_DMCR = dmcr | EMC_DMCR_RFSH;

    tmp = CFG_SDRAM_TREF / ns;
    tmp = tmp / 64 + 1;
    if (tmp > 0xff)
        tmp = 0xff;
    REG_EMC_RTCOR = tmp;
    REG_EMC_RTCNT = 0;
    REG_EMC_RTCSR = EMC_RTCSR_CKS_64;   /* Divisor is 64, CKO/64 */

    /* Wait for number of auto-refresh cycles */
    tmp = (cpu_clk / 1000000) * 1000;
    while (tmp--);

    /* Stage 3. Mode Register Set */
    REG_EMC_DMCR = dmcr0 | EMC_DMCR_RFSH | EMC_DMCR_MRSET;
    REG8(EMC_SDMR0 | sdmode) = 0;

    /* Set back to basic DMCR value */
    REG_EMC_DMCR = dmcr | EMC_DMCR_RFSH | EMC_DMCR_MRSET;

    /* everything is ok now */
}

void main_func(void)
{

    /*
     * Init gpio, serial, pll and sdram
     */
    gpio_init();
    serial_init();
    serial_puts("\nJZ x_loader version " VERSION "\n");
    serial_puts("Copyright 2009 by yajin<yajin@vm-kernel.org>\n");
    pll_init();
    sdram_init();
}

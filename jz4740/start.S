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

	.text
	.set noreorder
	.global startup
startup:
	/*
	 * Disable all interrupts
	 */
	la	$8, 0xB0001004		/* INTC_IMR */
	li	$9, 0xffffffff
	sw	$9, 0($8)

	/* 
	 * CU0=UM=EXL=IE=0, BEV=ERL=1, IP2~7=1
	 */
	li	$26, 0x0040FC04
	mtc0	$26, $12		/* CP0_STATUS */

	/* IV=1, use the specical interrupt vector (0x200) */
	li	$26, 0x00800000
	mtc0	$26, $13		/* CP0_CAUSE */

	/* Setup stack pointer */
	la	$29, 0x81000000

	/* Jump to the main routine */
	j	main_func
	nop
	.set reorder


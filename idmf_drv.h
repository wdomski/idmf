/*
 * Mecovis IntelliDAQ Multi-Function Series Driver
 * Copyright (C) 2011, Mecovis GmbH.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef __IDMF_DRV_H
#define __IDMF_DRV_H

#include <linux/pci.h>
#include <linux/device.h>
#include <linux/cdev.h>

#define MAX_BOARD_COUNT 6

/**
 * idmf_board
 * @pdev:	pci device structure
 * @base:	pointer to start of io memory
 */
struct idmf_board {
	struct list_head list;

	struct rtdm_device * dev;

	struct pci_dev	*pdev;

	u32 __iomem	*base;

	u32	init_flags;
};

#endif /* __IDMF_DRV_H */

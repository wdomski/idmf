/*
 * Copyright (C) 2015 Wojciech Domski <Wojciech.Domski@gmail.com>
 *
 * this driver is based on non-realtime driver for
 * Mecovis IntelliDAQ Multi-Function Series Driver.
 * It is a RTDM driver for Xenomai.
 *
 * Original copyright:
 *
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

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/fs.h>
#include <rtdm/rtdm_driver.h>

#include "idmf_drv.h"

MODULE_DESCRIPTION("Realtime Mecovis Aurora driver");
MODULE_AUTHOR ("Wojciech Domski");
MODULE_VERSION ("0.1");
MODULE_LICENSE ("GPL v2");

static LIST_HEAD( idmf_list);

/* module global variables */
static const char driver_name[] = "idmf-rtdm";

/* init flags */
#define INIT_PCI_ENABLE				0x0001
#define INIT_PCI_REQUEST_REGIONS	0x0002
#define INIT_PCI_IOMAP				0x0004
#define INIT_PCI_REQUEST_IRQ		0x0008
#define INIT_IDMF_MINOR_RESERVE		0x0010
#define INIT_CDEV_ADD				0x0020
#define INIT_DEVICE_CREATE			0x0040
#define INIT_CREATE_ATTRIBUTES		0x0080

#define REG_WRITE	0x10000000
#define REG_READ	0x20000000

int idmf_open(struct rtdm_dev_context *context, rtdm_user_info_t * user_info,
		int oflags);

int idmf_close(struct rtdm_dev_context *context, rtdm_user_info_t * user_info);

int idmf_ioctl(struct rtdm_dev_context *context, rtdm_user_info_t *user_info,
		unsigned int request, void __user *arg);

static void idmf_pci_remove(struct pci_dev *pdev);

static int idmf_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id);

//static long idmf_ioctl(struct rtdm_dev_context *context, rtdm_user_info_t *user_info,
//		unsigned int request, void *arg)
int idmf_ioctl(struct rtdm_dev_context *context, rtdm_user_info_t *user_info,
		unsigned int request, void *arg)
{
	struct idmf_board *board = NULL;
	u32 value;
	long retval = 0;

	board = (struct idmf_board *)(context->device->device_data);

	if(!board)
		goto leave;

	if (request & 0x03) {
		rtdm_printk( "idmf_drv: %s: request must be a multiple of 4\n",
				__PRETTY_FUNCTION__);
		retval = -EINVAL;
		goto leave;
	}

	if (request & REG_WRITE) {
		retval = copy_from_user(&value, arg, sizeof(value));
		if (retval) {
			rtdm_printk( "idmf_drv: %s: copy_from_user failed\n",
					__PRETTY_FUNCTION__);
			goto leave;
		}

		iowrite32(value, (u8 *)board->base + (request & 0xFFFC));
	}

	if (request & REG_READ) {
		value = ioread32((u8 *)board->base + (request & 0xFFFC));

		retval = copy_to_user(arg, &value, sizeof(value));
		if (retval) {
			rtdm_printk( "idmf_drv: %s: copy_to_user failed\n",
					__PRETTY_FUNCTION__);
			goto leave;
		}
	}

	return 0;

	leave:
	rtdm_printk( "idmf_drv: <--- %s (%ld)\n", __PRETTY_FUNCTION__, retval);
	return retval;
}

int idmf_open(struct rtdm_dev_context *context, rtdm_user_info_t * user_info,
		int oflags) {
	return 0;
}

int idmf_close(struct rtdm_dev_context *context, rtdm_user_info_t * user_info) {
	return 0;
}

static struct rtdm_device rtdm_idmf_device_tmpl = { .struct_version =
		RTDM_DEVICE_STRUCT_VER,

.device_flags = RTDM_NAMED_DEVICE, .context_size = 0, .device_name = "",

.open_nrt = idmf_open, .open_rt = idmf_open,

.ops = { .close_nrt = idmf_close, .close_rt = idmf_close, .ioctl_nrt =
		idmf_ioctl, .ioctl_rt = idmf_ioctl, },

.device_class = RTDM_CLASS_EXPERIMENTAL, .device_sub_class =
		RTDM_SUBCLASS_GENERIC, .profile_version = 1, .driver_name = driver_name,
		.driver_version = RTDM_DRIVER_VER(0, 1, 0), .peripheral_name =
				"idmf board", .provider_name = "Wojciech Domski", .proc_name =
				rtdm_idmf_device_tmpl.device_name, };

int pci_registered;

static void idmf_pci_remove(struct pci_dev *pdev) {
	struct list_head *ptr;
	struct idmf_board *idmfptr = NULL;
	struct idmf_board *board = (struct idmf_board *) pci_get_drvdata(pdev);

	rtdm_printk("idmf_drv: %s\n",
			__PRETTY_FUNCTION__);

	if (board->init_flags & INIT_PCI_IOMAP)
		pci_iounmap(pdev, board->base);

	if (board->init_flags & INIT_PCI_REQUEST_REGIONS)
		pci_release_regions(pdev);

	if (board->init_flags & INIT_PCI_ENABLE)
		pci_disable_device(pdev);

	list_for_each(ptr, &idmf_list)
	{
		idmfptr = list_entry(ptr, struct idmf_board, list);

		if( idmfptr == board)
		{
			list_del(ptr);

			kfree(idmfptr);

			--pci_registered;

			break;
		}
	}

}

static int idmf_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id) {
	struct idmf_board *board;
	void *base = NULL;
	int err = 0;

	rtdm_printk("idmf_drv: %s\n", __PRETTY_FUNCTION__);

	/* init and register idmf_board structure */
	board = kzalloc(sizeof(struct idmf_board), GFP_KERNEL);
	if (board == NULL) {
		rtdm_printk("idmf_drv: %s: kzalloc failed\n", __PRETTY_FUNCTION__);
		err = -ENOMEM;
		goto leave;
	}

	pci_set_drvdata(pdev, (void *) board);

	/* init pci device */
	err = pci_enable_device(pdev);
	if (err) {
		rtdm_printk("idmf_drv: %s: pci_enable_device failed\n",
				__PRETTY_FUNCTION__);
		goto leave;
	}
	board->init_flags |= INIT_PCI_ENABLE;

	/* enable bus-mastering on the device
	 pci_set_master(pdev); */

	/* init memory mapping */
	err = pci_request_regions(pdev, driver_name);
	if (err) {
		rtdm_printk("idmf_drv: %s: pci_request_regions failed\n",
				__PRETTY_FUNCTION__);
		goto leave;
	}
	board->init_flags |= INIT_PCI_REQUEST_REGIONS;

	base = pci_iomap(pdev, 0, 0x1000);
	if (base == NULL) {
		rtdm_printk("idmf_drv: %s: pci_iomap failed\n", __PRETTY_FUNCTION__);
		err = -EFAULT;
		goto leave;
	}
	board->init_flags |= INIT_PCI_IOMAP;

	board->base = base;

	board->pdev = pdev;

	list_add(&board->list, &idmf_list);

	leave:

	if (err)
		idmf_pci_remove(pdev);

	return err;
}
;

/* pci driver structures */
DEFINE_PCI_DEVICE_TABLE (pci_ids) = { { PCI_DEVICE(0x1172, 0x0004) }, { 0 } };

MODULE_DEVICE_TABLE( pci, pci_ids);

static struct pci_driver idmf_pci_driver =
		{ .name = (char *) driver_name, .id_table = pci_ids, .probe =
				idmf_pci_probe, .remove = idmf_pci_remove, };

static void idmf_pci_init(void) {
	if (pci_register_driver(&idmf_pci_driver) == 0)
		++pci_registered;
}

static void idmf_pci_cleanup(void) {

	if (pci_registered > 0)
		pci_unregister_driver(&idmf_pci_driver);
}

static int idmf_init(void) {
	int err = 0;
	int index = 0;

	struct list_head *ptr;
	struct idmf_board *idmfptr = NULL;
	struct rtdm_device * device;

	idmf_pci_init();

	list_for_each(ptr, &idmf_list)
	{
		idmfptr = list_entry(ptr, struct idmf_board, list);

		device = kzalloc(sizeof(struct rtdm_device), GFP_KERNEL);
		err = -ENOMEM;
		if (!device) {
			rtdm_printk(
					"idmf_drv: Couldn't allocate memory for device in %s\n",
					__PRETTY_FUNCTION__);
			goto cleanup_out;
		}

		memcpy(device, &rtdm_idmf_device_tmpl, sizeof(struct rtdm_device));
		snprintf(device->device_name, RTDM_MAX_DEVNAME_LEN, "idmf%d", index);

		device->proc_name = device->device_name;

		idmfptr->dev = device;

		rtdm_printk("idmf_drv: New device: %s\n", device->device_name);

		err = rtdm_dev_register(device);

		if (err) {
			rtdm_printk("idmf_drv: Couldn't register rtdm device in %s\n",
					__PRETTY_FUNCTION__);
			goto cleanup_out;
		}

		device->device_data = (void *)idmfptr;

		++index;

	}

	cleanup_out:

	return err;
}

static void idmf_cleanup(void) {

	struct list_head *ptr;
	struct idmf_board *idmfptr = NULL;

	list_for_each(ptr, &idmf_list)
	{
		idmfptr = list_entry(ptr, struct idmf_board, list);

		rtdm_printk("idmf_drv: unregister device %s in %s\n",
				idmfptr->dev->device_name, __PRETTY_FUNCTION__);
		rtdm_dev_unregister(idmfptr->dev, 1000);

		kfree(idmfptr->dev);
	}

	idmf_pci_cleanup();
}

module_init( idmf_init);
module_exit( idmf_cleanup);


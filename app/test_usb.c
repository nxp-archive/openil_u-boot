/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <usb.h>
#include <memalign.h>

#include <command.h>
#include <console.h>
#include <dm.h>
#include <dm/uclass-internal.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <part.h>

static int usb_stor_curr_dev = -1; /* current device */

static void usb_display_string(struct usb_device *dev, int index)
{
	ALLOC_CACHE_ALIGN_BUFFER(char, buffer, 256);

	if (index != 0) {
		if (usb_string(dev, index, &buffer[0], 256) > 0)
			printf("String: \"%s\"", buffer);
	}
}

static void usb_display_conf_desc(struct usb_config_descriptor *config,
				  struct usb_device *dev)
{
	printf("   Configuration: %d\n", config->bConfigurationValue);
	printf("   - Interfaces: %d %s%s%dmA\n", config->bNumInterfaces,
	       (config->bmAttributes & 0x40) ? "Self Powered " : "Bus Powered ",
	       (config->bmAttributes & 0x20) ? "Remote Wakeup " : "",
		config->bMaxPower*2);
	if (config->iConfiguration) {
		printf("   - ");
		usb_display_string(dev, config->iConfiguration);
		printf("\n");
	}
}

/* some display routines (info command) */
static char *usb_get_class_desc(unsigned char dclass)
{
	switch (dclass) {
	case USB_CLASS_PER_INTERFACE:
		return "See Interface";
	case USB_CLASS_AUDIO:
		return "Audio";
	case USB_CLASS_COMM:
		return "Communication";
	case USB_CLASS_HID:
		return "Human Interface";
	case USB_CLASS_PRINTER:
		return "Printer";
	case USB_CLASS_MASS_STORAGE:
		return "Mass Storage";
	case USB_CLASS_HUB:
		return "Hub";
	case USB_CLASS_DATA:
		return "CDC Data";
	case USB_CLASS_VENDOR_SPEC:
		return "Vendor specific";
	default:
		return "";
	}
}


static void usb_display_class_sub(unsigned char dclass, unsigned char subclass,
				  unsigned char proto)
{
	switch (dclass) {
	case USB_CLASS_PER_INTERFACE:
		printf("See Interface");
		break;
	case USB_CLASS_HID:
		printf("Human Interface, Subclass: ");
		switch (subclass) {
		case USB_SUB_HID_NONE:
			printf("None");
			break;
		case USB_SUB_HID_BOOT:
			printf("Boot ");
			switch (proto) {
			case USB_PROT_HID_NONE:
				printf("None");
				break;
			case USB_PROT_HID_KEYBOARD:
				printf("Keyboard");
				break;
			case USB_PROT_HID_MOUSE:
				printf("Mouse");
				break;
			default:
				printf("reserved");
				break;
			}
			break;
		default:
			printf("reserved");
			break;
		}
		break;
	case USB_CLASS_MASS_STORAGE:
		printf("Mass Storage, ");
		switch (subclass) {
		case US_SC_RBC:
			printf("RBC ");
			break;
		case US_SC_8020:
			printf("SFF-8020i (ATAPI)");
			break;
		case US_SC_QIC:
			printf("QIC-157 (Tape)");
			break;
		case US_SC_UFI:
			printf("UFI");
			break;
		case US_SC_8070:
			printf("SFF-8070");
			break;
		case US_SC_SCSI:
			printf("Transp. SCSI");
			break;
		default:
			printf("reserved");
			break;
		}
		printf(", ");
		switch (proto) {
		case US_PR_CB:
			printf("Command/Bulk");
			break;
		case US_PR_CBI:
			printf("Command/Bulk/Int");
			break;
		case US_PR_BULK:
			printf("Bulk only");
			break;
		default:
			printf("reserved");
			break;
		}
		break;
	default:
		printf("%s", usb_get_class_desc(dclass));
		break;
	}
}

static void usb_display_if_desc(struct usb_interface_descriptor *ifdesc,
				struct usb_device *dev)
{
	printf("     Interface: %d\n", ifdesc->bInterfaceNumber);
	printf("     - Alternate Setting %d, Endpoints: %d\n",
		ifdesc->bAlternateSetting, ifdesc->bNumEndpoints);
	printf("     - Class ");
	usb_display_class_sub(ifdesc->bInterfaceClass,
		ifdesc->bInterfaceSubClass, ifdesc->bInterfaceProtocol);
	printf("\n");
	if (ifdesc->iInterface) {
		printf("     - ");
		usb_display_string(dev, ifdesc->iInterface);
		printf("\n");
	}
}

static void usb_display_ep_desc(struct usb_endpoint_descriptor *epdesc)
{
	printf("     - Endpoint %d %s ", epdesc->bEndpointAddress & 0xf,
		(epdesc->bEndpointAddress & 0x80) ? "In" : "Out");
	switch ((epdesc->bmAttributes & 0x03)) {
	case 0:
		printf("Control");
		break;
	case 1:
		printf("Isochronous");
		break;
	case 2:
		printf("Bulk");
		break;
	case 3:
		printf("Interrupt");
		break;
	}
	printf(" MaxPacket %d", get_unaligned(&epdesc->wMaxPacketSize));
	if ((epdesc->bmAttributes & 0x03) == 0x3)
		printf(" Interval %dms", epdesc->bInterval);
	printf("\n");
}

/* main routine to diasplay the configs, interfaces and endpoints */
static void usb_display_config(struct usb_device *dev)
{
	struct usb_config *config;
	struct usb_interface *ifdesc;
	struct usb_endpoint_descriptor *epdesc;
	int i, ii;

	config = &dev->config;
	usb_display_conf_desc(&config->desc, dev);
	for (i = 0; i < config->no_of_if; i++) {
		ifdesc = &config->if_desc[i];
		usb_display_if_desc(&ifdesc->desc, dev);
		for (ii = 0; ii < ifdesc->no_of_ep; ii++) {
			epdesc = &ifdesc->ep_desc[ii];
			usb_display_ep_desc(epdesc);
		}
	}
	printf("\n");
}

static void usb_display_desc(struct usb_device *dev)
{
	uint packet_size = dev->descriptor.bMaxPacketSize0;

	if (dev->descriptor.bDescriptorType == USB_DT_DEVICE) {
		printf("%d: %s,  USB Revision %x.%x\n", dev->devnum,
		usb_get_class_desc(dev->config.if_desc[0].desc.bInterfaceClass),
				   (dev->descriptor.bcdUSB>>8) & 0xff,
				   dev->descriptor.bcdUSB & 0xff);

		if (strlen(dev->mf) || strlen(dev->prod) ||
		    strlen(dev->serial))
			printf(" - %s %s %s\n", dev->mf, dev->prod,
				dev->serial);
		if (dev->descriptor.bDeviceClass) {
			printf(" - Class: ");
			usb_display_class_sub(dev->descriptor.bDeviceClass,
					      dev->descriptor.bDeviceSubClass,
					      dev->descriptor.bDeviceProtocol);
			printf("\n");
		} else {
			printf(" - Class: (from Interface) %s\n",
			       usb_get_class_desc(
				dev->config.if_desc[0].desc.bInterfaceClass));
		}
		if (dev->descriptor.bcdUSB >= cpu_to_le16(0x0300))
			packet_size = 1 << packet_size;
		printf(" - PacketSize: %d  Configurations: %d\n",
			packet_size, dev->descriptor.bNumConfigurations);
		printf(" - Vendor: 0x%04x  Product 0x%04x Version %d.%d\n",
			dev->descriptor.idVendor, dev->descriptor.idProduct,
			(dev->descriptor.bcdDevice>>8) & 0xff,
			dev->descriptor.bcdDevice & 0xff);
	}
}

#ifdef CONFIG_DM_USB
static void usb_show_info(struct usb_device *udev)
{
	struct udevice *child;

	usb_display_desc(udev);
	usb_display_config(udev);
	for (device_find_first_child(udev->dev, &child);
	     child;
	     device_find_next_child(&child)) {
		if (device_active(child)) {
			udev = dev_get_parent_priv(child);
			usb_show_info(udev);
		}
	}
}
#endif

#ifdef CONFIG_DM_USB
typedef void (*usb_dev_func_t)(struct usb_device *udev);

static void usb_for_each_root_dev(usb_dev_func_t func)
{
	struct udevice *bus;

	for (uclass_find_first_device(UCLASS_USB, &bus);
		bus;
		uclass_find_next_device(&bus)) {
		struct usb_device *udev;
		struct udevice *dev;

		if (!device_active(bus))
			continue;

		device_find_first_child(bus, &dev);
		if (dev && device_active(dev)) {
			udev = dev_get_parent_priv(dev);
			func(udev);
		}
	}
}
#endif

void test_usb(void)
{
	int d;
	struct usb_device *udev = NULL;

#ifdef CONFIG_USB_STORAGE
	struct blk_desc *stor_dev;
#endif
	/* first to stop usb */
	usb_stop();

	/* start to init USB controller */
	bootstage_mark_name(BOOTSTAGE_ID_USB_START, "usb_start");
	if (usb_init() < 0)
		return;
	/* Driver model will probe the devices as they are found */
	/* try to recognize storage devices immediately */
	usb_stor_curr_dev = usb_stor_scan(1);

#ifndef CONFIG_DM_USB
#ifdef CONFIG_USB_KEYBOARD
	drv_usb_kbd_init();
#endif
#endif /* !CONFIG_DM_USB */

	/* to display the USB device tree */
	usb_show_tree();

#ifdef CONFIG_DM_USB
	usb_for_each_root_dev(usb_show_info);
#else
	/* display USB information */
	for (d = 0; d < USB_MAX_DEVICE; d++) {
		udev = usb_get_dev_index(d);
		if (udev == NULL)
			break;
		usb_display_desc(udev);
		usb_display_config(udev);
	}
#endif

	/* data read/write from/to USB mass storage device */
	if (usb_stor_curr_dev < 0)
		printf("no current device selected\n");
	else {
		unsigned long addr = 0x80000000;
		unsigned long blk  = 0x0;
		unsigned long cnt  = 0x10;
		unsigned long n;

		/* read the data to memory */
		printf("\nUSB read: device %d block # %ld, count %ld  ... ",
			usb_stor_curr_dev, blk, cnt);
		stor_dev = blk_get_devnum_by_type(IF_TYPE_USB,
			usb_stor_curr_dev);
		n = blk_dread(stor_dev, blk, cnt, (ulong *)addr);
		printf("%ld blocks read: %s\n", n,
			(n == cnt) ? "OK" : "ERROR");

		/* write the data to USB mass storage device */
		printf("\nUSB write: device %d block # %ld, count %ld  ... ",
			usb_stor_curr_dev, blk, cnt);
		stor_dev = blk_get_devnum_by_type(IF_TYPE_USB,
			usb_stor_curr_dev);
		n = blk_dwrite(stor_dev, blk, cnt, (ulong *)addr);
		printf("%ld blocks write: %s\n", n,
			(n == cnt) ? "OK" : "ERROR");
	}
}

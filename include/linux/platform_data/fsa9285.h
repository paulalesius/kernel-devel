/*
 * Copyright (C) 2015
 * Paul Sipot <paul@unnservice.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/i2c.h>

// --------------------------------------------------------------------------------- guard and definitions

#ifndef _FSA9825_H_
#define _FSA9825_H_

#define FSA9825_ATTACHED	1
#define FSA9825_DETACHED	0

// --------------------------------------------------------------------------------- structures

struct fsa9825_platform_data {
	void (*cfg_gpio) (void);
	void (*usb_cb) (u8 attached);
	void (*uart_cb) (u8 attached);
	void (*charger_cb) (u8 attached);
	void (*jig_cb) (u8 attached);
	void (*reset_cb) (void);
	void (*usb_power) (u8 on);
	int wakeup;
};

static const struct i2c_device_id fsa9285_id[] = {
	{"fsa9285", 0},
	{}
};

static DEVICE_ATTR(device, S_IRUGO, fsa9285_show_device, NULL);
static DEVICE_ATTR(switch, S_IRUGO | S_IWUSR, fsa9285_show_manualsw, fsa9285_set_manualsw);

static struct attribute *fsa9285_attributes[] = {
	&dev_attr_device.attr,
	&dev_attr_switch.attr,
	NULL
};

static const struct attribute_group fsa9285_group = {
	.attrs = fsa9285_attributes,
};

struct fsa9285_usb_switch {
	struct i2c_client				*client;
	struct fsa9825_platform_data	*pdata;
	struct usb_phy					*otg;
	struct extcon_dev				*edev;

	/* reg data */
	u8								cntl;
	u8								man_sw;
	u8								man_chg_cntl;

	bool							vbus_drive;
	bool							a_bus_drop;
};

#define FSA9285_EXTCON_SDP		"CHARGER_USB_SDP"
#define FSA9285_EXTCON_DCP		"CHARGER_USB_DCP"
#define FSA9285_EXTCON_CDP		"CHARGER_USB_CDP"
#define FSA9285_EXTCON_ACA		"CHARGER_USB_ACA"
#define FSA9285_EXTCON_DOCK		"Dock"
#define FSA9285_EXTCON_USB_HOST		"USB-Host"

static const char *fsa9285_extcon_cable[] = {
	FSA9285_EXTCON_SDP,
	FSA9285_EXTCON_DCP,
	FSA9285_EXTCON_CDP,
	FSA9285_EXTCON_ACA,
	FSA9285_EXTCON_DOCK,
	FSA9285_EXTCON_USB_HOST,
	NULL,
};

// --------------------------------------------------------------------------------- prototypes

/**
 * bind to device
 */
static int fsa9285_probe(struct i2c_client *client, const struct i2c_device_id *id);

/**
 * Unbind from device
 */
static int fsa9285_remove(struct i2c_client *client);

static ssize_t fsa9285_show_device(struct device *dev, struct device_attribute *attr, char *buf);

static ssize_t fsa9285_show_manualsw(struct device *dev, struct device_attribute *attr, char *buf);

static ssize_t fsa9285_set_manualsw(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

#endif /* _FSA9825_H_ */

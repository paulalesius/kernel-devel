/*
 * fsa825.c - fsa9285 micro USB switch device driver
 *
 * Copyright (C) 2010 Samsung Electronics
 * Paul Sipot <paul@unnservice.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/platform_data/fsa9285.h>
#include <linux/extcon.h>

/* FSA9285 I2C registers */
#define FSA9285_REG_DEVID		0x01
#define FSA9285_REG_TIMING		0x08

/* Interrupt 1 */
#define INT_DETACH				(1 << 1)
#define INT_ATTACH				(1 << 0)

#define DEVID_VALUE				0x10
#define TIMING_500MS			0x6
#define MAX_RETRY				3

extern void *fsa9825_platform_data(void);

static int fsa9285_write_reg(struct i2c_client *client, int reg, int value) {

	int ret;
	int retry;

	for(retry = 0; retry < MAX_RETRY; retry++) {
		ret = i2c_smbus_write_byte_data(client, reg, value);

		if (ret < 0) {
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
		} else {
			break;
		}
	}

	return ret;
}

static int fsa9285_read_reg(struct i2c_client *client, int reg) {

	int ret;
	int retry;

	for(retry = 0; retry < MAX_RETRY; retry++) {
		ret = i2c_smbus_read_byte_data(client, reg);

		if (ret < 0) {
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
		} else {
			break;
		}
	}

	return ret;
}

//----------------------------------------------------------------------------------------

static int fsa9285_irq_init(struct fsa9280_usb_switch *usb_switch) {
	// @TODO implement
	return -1;
}

//----------------------------------------------------------------------------------------

/**
 * Verify i2c functionality, and chip ID
 */
static int fsa9285_probe_check(struct i2c_client *client) {

	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

	if(!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		return -EIO;
	}

	int ret = fsa9285_read_reg(client, FSA9285_REG_DEVID);
	if(ret < 0 || ret != DEVID_VALUE) {
		dev_err(&client->dev, "fsa chip ID check failed:%d\n", ret);
		return -ENODEV;
	}

	return ret;
}

static int fsa9285_probe_extcon(struct i2c_client *client, struct fsa9285_usb_switch *usb_switch) {

	usb_switch->edev->name = "fsa9285";
	usb_switch->edev->supported_cable = fsa9285_extcon_cable;
	if(extcon_dev_register(usb_switch->edev, &client->dev)) {
		extcon_dev_unregister(usb_switch->edev);
		dev_err(&client->dev, "Extcon registration failed\n");
		return -ENODEV;
	}

	return 0;
}

/**
 * Register for otg
 */
static int fsa9285_probe_otg(struct i2c_client *client, struct fsa9285_usb_switch *usb_switch) {


	usb_switch->otg = usb_get_phy(USB_PHY_TYPE_USB2);
	if(!usb_switch->otg) {
		dev_warn(&client->dev, "Failed to get otg transciever!!\n");
		return -ENODEV;
	}
	return 0;
}

static int fsa9285_probe(struct i2c_client *client, const struct i2c_device_id *id) {

	int ret = 0;

	if((ret = fsa9285_probe_check(client)) < 0) {
		return -ENODEV;
	}

	// Allocations
	struct fsa9285_usb_switch *usb_switch = kzalloc(sizeof(struct fsa9285_usb_switch), GFP_KERNEL);
	if(!usb_switch) {
		kfree(usb_switch);
		dev_err(&client->dev, "Memory allocation failed\n");
		return -ENOMEM;
	}
	usb_switch->edev = kzalloc(sizeof(struct extcon_dev), GFP_KERNEL);
	if(!usb_switch->edev) {
		kfree(usb_switch->edev);
		dev_err(&client->dev, "Memory allocation failed\n");
		return -ENOMEM;
	}

	usb_switch->client = client;
	usb_switch->pdata = client->dev.platform_data;
	// Initialise i2c client with usb_switch client data
	i2c_set_clientdata(client, usb_switch);

	if(fsa9285_probe_extcon(client, usb_switch) < 0) {
		goto usb_switch_failure;
	}

	if(fsa9285_probe_otg(client, usb_switch) < 0) {
		goto usb_switch_failure;
	}

	usb_switch->

	/**
	 * Initialise extcon data structures
	 */

	ret = fsa9285_irq_init(usb_switch);
	if(ret) {
		goto usb_switch_failure;
	}

	ret = sysfs_create_group(&client->dev.kobj, &fsa9285_group);
	if(ret) {
		dev_err(&client->dev, "failed to create fsa9480 attribute group\n");
		goto fail2;
	}

	/**
	 * Relax ADC detect time to 500ms, like in fsa9480
	 */
	fsa9285_write_reg(client, FSA9285_REG_TIMING, TIMING_500MS);

	if(usb_switch->pdata->reset_cb) {
		usb_switch->pdata->reset_cb();
	}

	fsa9285_detect_dev(usb_switch, INT_ATTACH);
	//pm_runtime_set_active(&client->dev);
	//pm_runtime_put_noidle(&chip->client->dev);
	//pm_schedule_suspend(&chip->client->dev, MSEC_PER_SEC);

	return 0;

fail2:
	if (client->irq)
		free_irq(client->irq, usb_switch);
usb_switch_failure:
	kfree(usb_switch->edev);
	kfree(usb_switch);
	return ret;
}

static int fsa9285_remove(struct i2c_client *client) {

	struct fsa9285_usb_switch *usb_switch = i2c_get_clientdata(client);
	if(client->irq) {
		free_irq(client->irq, usb_switch);
	}

	sysfs_remove_group(&client->dev.kobj, &fsa9285_group);
	device_init_wakeup(&client->dev, 0);
	kfree(usb_switch);
	return 0;
}

//----------------------------------------------------------------------------------------

// .pm is null since we don't implement it, for now...
static struct i2c_driver fsa9285_i2c_driver = {
	.driver = {
		.name = "fsa9285",
		// power management operations to be performed when entering low power modes, like suspend or resume
		.pm = NULL,
	},
	.probe = fsa9285_probe,
	.remove = fsa9285_remove,
	.id_table = fsa9285_id,
};

MODULE_DEVICE_TABLE(i2c, fsa9285_id);
// module_i2c_driver replaces module init/module exit
module_i2c_driver(fsa9285_i2c_driver);

MODULE_AUTHOR("Paul Sipot <paul@unnservice.com>");
MODULE_DESCRIPTION("fsa9285 USB Switch driver");
MODULE_LICENSE("GPL");


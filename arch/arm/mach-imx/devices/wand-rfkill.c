/*
 * arch/arm/mach-imx/devices/wand-rfkill.c
 *
 * Copyright (C) 2013 Vladimir Ermakov <vooon341@gmail.com>
 *
 * based on net/rfkill/rfkill-gpio.c
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>


struct wand_rfkill_data {
	struct rfkill *rfkill_dev;
	int shutdown_gpio;
	const char *shutdown_name;
};

static int wand_rfkill_set_block(void *data, bool blocked)
{
	struct wand_rfkill_data *rfkill = data;

	pr_debug("wandboard-rfkill: set block %d\n", blocked);

	if (blocked) {
		if (gpio_is_valid(rfkill->shutdown_gpio))
			gpio_direction_output(rfkill->shutdown_gpio, 0);
	} else {
		if (gpio_is_valid(rfkill->shutdown_gpio))
			gpio_direction_output(rfkill->shutdown_gpio, 1);
	}

	return 0;
}

static const struct rfkill_ops wand_rfkill_ops = {
	.set_block = wand_rfkill_set_block,
};

static int wand_rfkill_wifi_probe(struct device *dev,
		struct device_node *np,
		struct wand_rfkill_data *rfkill,
		int wand_rev)
{
	int ret;
	int wl_ref_on, wl_rst_n, wl_reg_on, wl_wake, wl_host_wake;

	wl_reg_on = of_get_named_gpio(np, "wifi-reg-on", 0);
	wl_wake = of_get_named_gpio(np, "wifi-wake", 0);
	wl_host_wake = of_get_named_gpio(np, "wifi-host-wake", 0);

	if(wand_rev){
		wl_ref_on = of_get_named_gpio(np, "wifi-ref-on-revc1", 0);
		wl_rst_n = wl_reg_on;
	}
	else {
		wl_ref_on = of_get_named_gpio(np, "wifi-ref-on", 0);
		wl_rst_n = of_get_named_gpio(np, "wifi-rst-n", 0);
	}

	if (!gpio_is_valid(wl_rst_n) || !gpio_is_valid(wl_ref_on) ||
			!gpio_is_valid(wl_reg_on) || !gpio_is_valid(wl_wake) ||
			!gpio_is_valid(wl_host_wake)) {

		dev_err(dev, "incorrect wifi gpios (%d %d %d %d %d)\n",
				wl_rst_n, wl_ref_on, wl_reg_on, wl_wake, wl_host_wake);
		return -EINVAL;
	}

	dev_info(dev, "initialize wifi chip\n");

	gpio_request(wl_rst_n, "wl_rst_n");
	gpio_direction_output(wl_rst_n, 0);
	msleep(11);
	gpio_set_value(wl_rst_n, 1);

	gpio_request(wl_ref_on, "wl_ref_on");
	gpio_direction_output(wl_ref_on, 1);

	gpio_request(wl_reg_on, "wl_reg_on");
	gpio_direction_output(wl_reg_on, 1);

	gpio_request(wl_wake, "wl_wake");
	gpio_direction_output(wl_wake, 1);

	gpio_request(wl_host_wake, "wl_host_wake");
	gpio_direction_input(wl_host_wake);

	rfkill->shutdown_name = "wifi_shutdown";
	rfkill->shutdown_gpio = wl_reg_on;

	rfkill->rfkill_dev = rfkill_alloc("wifi-rfkill", dev, RFKILL_TYPE_WLAN,
			&wand_rfkill_ops, rfkill);
	if (!rfkill->rfkill_dev) {
		ret = -ENOMEM;
		goto wifi_fail_free_gpio;
	}

	ret = rfkill_register(rfkill->rfkill_dev);
	if (ret < 0)
		goto wifi_fail_unregister;

	dev_info(dev, "wifi-rfkill registered.\n");

	return 0;

wifi_fail_unregister:
	rfkill_destroy(rfkill->rfkill_dev);
wifi_fail_free_gpio:
	if (gpio_is_valid(wl_rst_n))     gpio_free(wl_rst_n);
	if (gpio_is_valid(wl_ref_on))    gpio_free(wl_ref_on);
	if (gpio_is_valid(wl_reg_on))    gpio_free(wl_reg_on);
	if (gpio_is_valid(wl_wake))      gpio_free(wl_wake);
	if (gpio_is_valid(wl_host_wake)) gpio_free(wl_host_wake);

	return ret;
}

static int wand_rfkill_bt_probe(struct device *dev,
		struct device_node *np,
		struct wand_rfkill_data *rfkill,
		int wand_rev)
{
	int ret;
	int bt_on, bt_wake, bt_host_wake;

	if(wand_rev) {
	    bt_on = of_get_named_gpio(np, "bluetooth-on-revc1", 0);
	    bt_wake = of_get_named_gpio(np, "bluetooth-wake-revc1", 0);
	    bt_host_wake = of_get_named_gpio(np, "bluetooth-host-wake-revc1", 0);
	}
	else{
	    bt_on = of_get_named_gpio(np, "bluetooth-on", 0);
	    bt_wake = of_get_named_gpio(np, "bluetooth-wake", 0);
	    bt_host_wake = of_get_named_gpio(np, "bluetooth-host-wake", 0);
	}

	if (!gpio_is_valid(bt_on) || !gpio_is_valid(bt_wake) ||
			!gpio_is_valid(bt_host_wake)) {

		dev_err(dev, "incorrect bt gpios (%d %d %d)\n",
				bt_on, bt_wake, bt_host_wake);
		return -EINVAL;
	}

	dev_info(dev, "initialize bluetooth chip\n");

	gpio_request(bt_on, "bt_on");
	gpio_direction_output(bt_on, 0);
	msleep(11);
	gpio_set_value(bt_on, 1);

	gpio_request(bt_wake, "bt_wake");
	gpio_direction_output(bt_wake, 1);

	gpio_request(bt_host_wake, "bt_host_wake");
	gpio_direction_input(bt_host_wake);

	rfkill->shutdown_name = "bluetooth_shutdown";
	rfkill->shutdown_gpio = bt_on;

	rfkill->rfkill_dev = rfkill_alloc("bluetooth-rfkill", dev, RFKILL_TYPE_BLUETOOTH,
			&wand_rfkill_ops, rfkill);
	if (!rfkill->rfkill_dev) {
		ret = -ENOMEM;
		goto bt_fail_free_gpio;
	}

	ret = rfkill_register(rfkill->rfkill_dev);
	if (ret < 0)
		goto bt_fail_unregister;

	dev_info(dev, "bluetooth-rfkill registered.\n");

	return 0;

bt_fail_unregister:
	rfkill_destroy(rfkill->rfkill_dev);
bt_fail_free_gpio:
	if (gpio_is_valid(bt_on))        gpio_free(bt_on);
	if (gpio_is_valid(bt_wake))      gpio_free(bt_wake);
	if (gpio_is_valid(bt_host_wake)) gpio_free(bt_host_wake);

	return ret;
}

static int wand_rfkill_probe(struct platform_device *pdev)
{
	struct wand_rfkill_data *rfkill;
	struct pinctrl *pinctrl;
	int ret;
	int wand_rev_gpio;
	int wand_rev;

	dev_info(&pdev->dev, "Wandboard rfkill initialization\n");

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "no device tree node\n");
		return -ENODEV;
	}

	rfkill = kzalloc(sizeof(*rfkill) * 2, GFP_KERNEL);
	if (!rfkill)
		return -ENOMEM;

	pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR(pinctrl)) {
		int ret = PTR_ERR(pinctrl);
		dev_err(&pdev->dev, "failed to get default pinctrl: %d\n", ret);
		return ret;
	}

	/* GPIO for detecting C1 revision of Wandboard */
	wand_rev_gpio = of_get_named_gpio(pdev->dev.of_node, "wand-rev-gpio", 0);
	if (!gpio_is_valid(wand_rev_gpio)) {

		dev_err(&pdev->dev, "incorrect Wandboard revision check gpio (%d)\n",
				wand_rev_gpio);
		return -EINVAL;
	}

	gpio_request(wand_rev_gpio, "wand-rev-gpio");
	dev_info(&pdev->dev, "initialized Wandboard revision check gpio (%d)\n",
			wand_rev_gpio);
	gpio_direction_input(wand_rev_gpio);

	/* Check Wandboard revision */
	wand_rev = gpio_get_value(wand_rev_gpio);
	if(wand_rev)
		dev_info(&pdev->dev,"wandboard is rev C1\n");
	else
		dev_info(&pdev->dev,"wandboard is rev B0\n");

	/* setup WiFi */
	ret = wand_rfkill_wifi_probe(&pdev->dev, pdev->dev.of_node, &rfkill[0], wand_rev);
	if (ret < 0)
		goto fail_free_rfkill;

	/* setup bluetooth */
	ret = wand_rfkill_bt_probe(&pdev->dev, pdev->dev.of_node, &rfkill[1], wand_rev);
	if (ret < 0)
		goto fail_unregister_wifi;

	platform_set_drvdata(pdev, rfkill);

	return 0;

fail_unregister_wifi:
	if (rfkill[1].rfkill_dev) {
		rfkill_unregister(rfkill[1].rfkill_dev);
		rfkill_destroy(rfkill[1].rfkill_dev);
	}

	/* TODO free gpio */

fail_free_rfkill:
	kfree(rfkill);

	return ret;
}

static int wand_rfkill_remove(struct platform_device *pdev)
{
	struct wand_rfkill_data *rfkill = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "Module unloading\n");

	if (!rfkill)
		return 0;

	/* WiFi */
	if (gpio_is_valid(rfkill[0].shutdown_gpio))
		gpio_free(rfkill[0].shutdown_gpio);

	rfkill_unregister(rfkill[0].rfkill_dev);
	rfkill_destroy(rfkill[0].rfkill_dev);

	/* Bt */
	if (gpio_is_valid(rfkill[1].shutdown_gpio))
		gpio_free(rfkill[1].shutdown_gpio);

	rfkill_unregister(rfkill[1].rfkill_dev);
	rfkill_destroy(rfkill[1].rfkill_dev);

	kfree(rfkill);

	return 0;
}

static struct of_device_id wand_rfkill_match[] = {
	{ .compatible = "wand,imx6q-wandboard-rfkill", },
	{ .compatible = "wand,imx6dl-wandboard-rfkill", },
	{ .compatible = "wand,imx6qdl-wandboard-rfkill", },
	{}
};

static struct platform_driver wand_rfkill_driver = {
	.driver = {
		.name = "wandboard-rfkill",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(wand_rfkill_match),
	},
	.probe = wand_rfkill_probe,
	.remove = wand_rfkill_remove
};

module_platform_driver(wand_rfkill_driver);

MODULE_AUTHOR("Vladimir Ermakov <vooon341@gmail.com>");
MODULE_DESCRIPTION("Wandboard rfkill driver");
MODULE_LICENSE("GPL v2");

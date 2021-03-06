/* Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "[LGE_BATT_ID] %s : " fmt, __func__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#include <linux/string.h>

#include <soc/qcom/lge/lge_battery_id_checker.h>
#include <soc/qcom/lge/power/lge_power_class.h>

#define MODULE_NAME "lge_battery_id"

static int lge_battery_info = BATT_ID_UNKNOWN;

struct lge_battery_id{
	struct lge_power lge_batt_id_lpc;
	struct lge_power *lge_cd_lpc;
	int battery_id_info;
	int is_factory_cable;
};
static bool is_lge_batt_valid(struct lge_battery_id *chip)
{
#ifdef CONFIG_LGE_PM_LGE_POWER_CLASS_CABLE_DETECT
	if (chip->is_factory_cable == 1)
		return true;
#endif

	if (chip->battery_id_info == BATT_ID_DS2704_N ||
			chip->battery_id_info == BATT_ID_DS2704_L ||
			chip->battery_id_info == BATT_ID_DS2704_C ||
			chip->battery_id_info == BATT_ID_ISL6296_N ||
			chip->battery_id_info == BATT_ID_ISL6296_L ||
			chip->battery_id_info == BATT_ID_ISL6296_C ||
			chip->battery_id_info == BATT_ID_RA4301_VC0 ||
			chip->battery_id_info == BATT_ID_RA4301_VC1 ||
			chip->battery_id_info == BATT_ID_RA4301_VC2 ||
			chip->battery_id_info == BATT_ID_SW3800_VC0 ||
			chip->battery_id_info == BATT_ID_SW3800_VC1 ||
			chip->battery_id_info == BATT_ID_SW3800_VC2)
		return true;

	return false;
}

static int read_lge_battery_id(struct lge_battery_id *chip)
{
	return chip->battery_id_info;
}

static bool get_prop_batt_id_for_aat(struct lge_battery_id *chip)
{
	static int check_batt_id;
	if (read_lge_battery_id(chip))
		check_batt_id = 1;
	else
		check_batt_id = 0;
	return check_batt_id;
}

static enum lge_power_property lge_power_lge_batt_id_properties[] = {
	LGE_POWER_PROP_BATTERY_ID_CHECKER,
	LGE_POWER_PROP_VALID_BATT,
	LGE_POWER_PROP_CHECK_BATT_ID_FOR_AAT,
	LGE_POWER_PROP_TYPE,
};
static enum lge_power_property
lge_power_lge_batt_id_uevent_properties[] = {
	LGE_POWER_PROP_VALID_BATT,
};
static int lge_power_lge_batt_id_get_property(struct lge_power *lpc,
			enum lge_power_property lpp,
			union lge_power_propval *val)
{
	int ret_val = 0;

	struct lge_battery_id *chip
			= container_of(lpc,
					struct lge_battery_id, lge_batt_id_lpc);
	switch (lpp) {
	case LGE_POWER_PROP_BATTERY_ID_CHECKER:
		val->intval = read_lge_battery_id(chip);
		break;

	case LGE_POWER_PROP_VALID_BATT:
		val->intval = is_lge_batt_valid(chip);
		break;

	case LGE_POWER_PROP_CHECK_BATT_ID_FOR_AAT:
		val->intval = get_prop_batt_id_for_aat(chip);
		break;

	case LGE_POWER_PROP_TYPE:
		val->intval = 0;
		break;

	default:
		ret_val = -EINVAL;
		break;
	}

	return ret_val;
}

static void lge_batt_id_external_lge_power_changed(struct lge_power *lpc)
{
	int rc = 0;
	union lge_power_propval lge_val = {0,};
	struct lge_battery_id *chip
			= container_of(lpc,
					struct lge_battery_id, lge_batt_id_lpc);

	if (!chip->lge_cd_lpc) {
		chip->lge_cd_lpc = lge_power_get_by_name("cable_detect");
		if (!chip->lge_cd_lpc) {
			pr_err("%s : cable_detect is not yet ready\n", __func__);
			return;
		}
	}
	rc = chip->lge_cd_lpc->get_property(chip->lge_cd_lpc,
			LGE_POWER_PROP_IS_FACTORY_CABLE, &lge_val);
	pr_info("factory cable : %d\n", lge_val.intval);
	chip->is_factory_cable = lge_val.intval;
}

static int lge_battery_id_probe(struct platform_device *pdev)
{
	struct lge_battery_id *battery_id;
	struct lge_power *lge_power_batt_id;
	int ret;
	pr_info("LG Battery ID probe Start~!!\n");
	battery_id = kzalloc(sizeof(struct lge_battery_id), GFP_KERNEL);

	if(!battery_id){
		pr_err("lge_battery_id memory allocation failed.\n");
		return -ENOMEM;
	}

	battery_id->battery_id_info = lge_battery_info;
	pr_info("Battery info : %d\n", battery_id->battery_id_info);
	battery_id->is_factory_cable = 0;
	lge_power_batt_id = &battery_id->lge_batt_id_lpc;

	lge_power_batt_id->name = "batt_id";

	lge_power_batt_id->properties = lge_power_lge_batt_id_properties;
	lge_power_batt_id->num_properties
		= ARRAY_SIZE(lge_power_lge_batt_id_properties);
	lge_power_batt_id->get_property
		= lge_power_lge_batt_id_get_property;
	lge_power_batt_id->external_lge_power_changed
		= lge_batt_id_external_lge_power_changed;
	lge_power_batt_id->uevent_properties
		= lge_power_lge_batt_id_uevent_properties;
	lge_power_batt_id->num_uevent_properties
		= ARRAY_SIZE(lge_power_lge_batt_id_uevent_properties);

	ret = lge_power_register(&pdev->dev, lge_power_batt_id);
	if (ret < 0) {
		pr_err("[LGE_CC] Failed to register lge power class: %d\n",
			ret);
		goto err_free;
	}

	pr_info("LG Battery ID probe done~!!\n");

	return 0;
err_free:
	kfree(battery_id);
	return ret;
}

#ifdef CONFIG_OF
static struct of_device_id lge_battery_id_match_table[] = {
	{.compatible = "lge,battery_id"},
	{},
};
#endif
static int lge_battery_id_remove(struct platform_device *pdev)
{
	struct lge_battery_id *battery_id = platform_get_drvdata(pdev);

	lge_power_unregister(&battery_id->lge_batt_id_lpc);

	platform_set_drvdata(pdev, NULL);
	kfree(battery_id);
	return 0;
}

static struct platform_driver lge_battery_id_driver = {
	.probe = lge_battery_id_probe,
	.remove = lge_battery_id_remove,
	.driver = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
	.of_match_table = lge_battery_id_match_table,
#endif
	},
};

static int __init lge_battery_id_init(void)
{
	return platform_driver_register(&lge_battery_id_driver);
}

static void lge_battery_id_exit(void)
{
	platform_driver_unregister(&lge_battery_id_driver);
}

fs_initcall(lge_battery_id_init);
module_exit(lge_battery_id_exit);



static int __init battery_information_setup(char *batt_info)
{
	if (!strcmp(batt_info, "DS2704_N"))
		lge_battery_info = BATT_ID_DS2704_N;
	else if (!strcmp(batt_info, "DS2704_L"))
		lge_battery_info = BATT_ID_DS2704_L;
	else if (!strcmp(batt_info, "DS2704_C"))
		lge_battery_info = BATT_ID_DS2704_C;
	else if (!strcmp(batt_info, "ISL6296_N"))
		lge_battery_info = BATT_ID_ISL6296_N;
	else if (!strcmp(batt_info, "ISL6296_L"))
		lge_battery_info = BATT_ID_ISL6296_L;
	else if (!strcmp(batt_info, "ISL6296_C"))
		lge_battery_info = BATT_ID_ISL6296_C;
	else if (!strcmp(batt_info, "RA4301_VC0"))
		lge_battery_info = BATT_ID_RA4301_VC0;
	else if (!strcmp(batt_info, "RA4301_VC1"))
		lge_battery_info = BATT_ID_RA4301_VC1;
	else if (!strcmp(batt_info, "RA4301_VC2"))
		lge_battery_info = BATT_ID_RA4301_VC2;
	else if (!strcmp(batt_info, "SW3800_VC0"))
		lge_battery_info = BATT_ID_SW3800_VC0;
	else if (!strcmp(batt_info, "SW3800_VC1"))
		lge_battery_info = BATT_ID_SW3800_VC1;
	else if (!strcmp(batt_info, "SW3800_VC2"))
		lge_battery_info = BATT_ID_SW3800_VC2;
	else if (!strcmp(batt_info, "NOT_PRESENT"))
		lge_battery_info = BATT_NOT_PRESENT;
	else
		lge_battery_info = BATT_ID_UNKNOWN;

	pr_info("Battery : %s %d\n", batt_info, lge_battery_info);

	return 1;
}

__setup("lge.battid=", battery_information_setup);

MODULE_DESCRIPTION("LGE power monitor class");
MODULE_AUTHOR("Daeho Choi <daeho.choi@lge.com>");
MODULE_LICENSE("GPL");

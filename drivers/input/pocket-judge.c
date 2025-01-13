/*
 * Copyright 2017 Paranoid Android
 *			 2022 iusmac <iusico.maxim@libero.it>
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/string.h>

#include "pocket-judge.h"

#ifdef CONFIG_TOUCHSCREEN_FOCALTECH
#include "touchscreen/focaltech_spi/focaltech_core.h"
#endif

/**
 * This driver maintains a sysfs interface used by the pocket bridge system
 * service. It enables and disables interrupts based on pocket state to
 * optimize battery consumption in-pocket.
 *
 * @author Chris Lahaye
 * @hide
 */

#ifdef CONFIG_TOUCHSCREEN_FOCALTECH
extern bool fts_spi_ts_probed;
#endif

static char pocket_judge_inpocket_state = '0';
static bool pocket_judge_inpocket = false;

static void pocket_judge_update(void)
{
#ifdef CONFIG_TOUCHSCREEN_FOCALTECH
	if (fts_spi_ts_probed)
		fts_spi_ts_inpocket_set(pocket_judge_inpocket);
#endif
}

bool pocket_judge_isInPocket(void) {
	return pocket_judge_inpocket;
}

void pocket_judge_forceDisable(void) {
	pocket_judge_inpocket = false;
	// SPECIAL STATE: can only be set internally (ex. by power button driver)
	// to notify clients that "safe door" to exit pocket lock was triggered.
	pocket_judge_inpocket_state = '2';
	pocket_judge_update();
}

static ssize_t inpocket_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	return sprintf(buf, "%c\n", pocket_judge_inpocket_state);
}

static ssize_t inpocket_store(struct device *dev, struct device_attribute *attr,
			      const char *val, size_t size)
{
    if (!val)
		return size;

	switch (val[0]) {
		case '0': // disable in-pocket
			// Save disabled (neutral) state and return here as we're in
			// special state.
			if (pocket_judge_inpocket_state == '2') {
				pocket_judge_inpocket_state = val[0];
				return size;
			}
			pocket_judge_inpocket = false;
			break;
		case '1': // enable in-pocket
			pocket_judge_inpocket = true;
			break;
		default:
			return size;
	}

	if (pocket_judge_inpocket_state != val[0]) {
		pocket_judge_inpocket_state = val[0];
		pocket_judge_update();
	}

	return size;
}

static DEVICE_ATTR_RW(inpocket);

static struct attribute *pocket_judge_class_attrs[] = {
	&dev_attr_inpocket.attr,
	NULL,
};

static const struct attribute_group pocket_judge_attr_group = {
	.attrs = pocket_judge_class_attrs,
};

static struct kobject *pocket_judge_kobj;

static int __init pocket_judge_init(void)
{
	ssize_t ret;

	pocket_judge_kobj = kobject_create_and_add("pocket_judge", kernel_kobj);
	if (!pocket_judge_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(pocket_judge_kobj, &pocket_judge_attr_group);
	if (ret)
		kobject_put(pocket_judge_kobj);

	return ret;
}

static void __exit pocket_judge_exit (void)
{
	kobject_put(pocket_judge_kobj);
}

module_init(pocket_judge_init);
module_exit(pocket_judge_exit);

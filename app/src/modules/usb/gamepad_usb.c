/*
 * Copyright (c) 2026 Mecha Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <zephyr/app_version.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/zbus/zbus.h>

#include "events/gamepad_events.h"
#include "modules/usb/gamepad_usb.h"

LOG_MODULE_REGISTER(gamepad_usb, CONFIG_COMET_GAMEPAD_LOG_LEVEL);

/*
 * The device release number is the firmware version as BCD digits MM.m.p, so 0.1.0 is 0x0010.
 * The major version has two digits, the minor version and the patch level have one.
 */
BUILD_ASSERT(APP_VERSION_MAJOR <= 99 && APP_VERSION_MINOR <= 9 && APP_PATCHLEVEL <= 9,
	     "The firmware version does not fit in the USB device release number");

#define GAMEPAD_USB_BCD_VERSION                                                                    \
	((((APP_VERSION_MAJOR / 10) << 12) | ((APP_VERSION_MAJOR % 10) << 8)) |                    \
	 (APP_VERSION_MINOR << 4) | APP_PATCHLEVEL)

/* The gamepad is powered from the USB bus and does not support remote wakeup yet. */
#define GAMEPAD_USB_ATTRIBUTES 0

USBD_DEVICE_DEFINE(gamepad_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   CONFIG_COMET_GAMEPAD_USB_VID, CONFIG_COMET_GAMEPAD_USB_PID);

USBD_DESC_LANG_DEFINE(gamepad_lang);
USBD_DESC_MANUFACTURER_DEFINE(gamepad_manufacturer, CONFIG_COMET_GAMEPAD_USB_MANUFACTURER);
USBD_DESC_PRODUCT_DEFINE(gamepad_product, CONFIG_COMET_GAMEPAD_USB_PRODUCT);
USBD_DESC_SERIAL_NUMBER_DEFINE(gamepad_serial);

USBD_DESC_CONFIG_DEFINE(gamepad_fs_cfg_desc, "FS Configuration");
USBD_CONFIGURATION_DEFINE(gamepad_fs_config, GAMEPAD_USB_ATTRIBUTES,
			  CONFIG_COMET_GAMEPAD_USB_MAX_POWER, &gamepad_fs_cfg_desc);

/* True while the host has set a configuration. Only used in the USB message callback. */
static bool configured;

static void gamepad_usb_publish(enum usb_state state)
{
	const struct usb_state_msg msg = {.state = state};
	int ret;

	ret = zbus_chan_pub(&usb_state_chan, &msg, K_MSEC(100));
	if (ret < 0) {
		LOG_WRN("Failed to publish the USB state (%d)", ret);
	}
}

static void gamepad_usb_msg_cb(struct usbd_context *const ctx, const struct usbd_msg *const msg)
{
	LOG_DBG("USB device message: %s", usbd_msg_type_string(msg->type));

	switch (msg->type) {
	case USBD_MSG_VBUS_READY:
		/* Only a controller that detects VBUS waits for it to be enabled. */
		if (usbd_can_detect_vbus(ctx) && usbd_enable(ctx) != 0) {
			LOG_ERR("Failed to enable the USB device");
		}
		gamepad_usb_publish(USB_STATE_CONNECTED);
		break;
	case USBD_MSG_VBUS_REMOVED:
		if (usbd_can_detect_vbus(ctx) && usbd_disable(ctx) != 0) {
			LOG_ERR("Failed to disable the USB device");
		}
		configured = false;
		gamepad_usb_publish(USB_STATE_DISCONNECTED);
		break;
	case USBD_MSG_RESET:
		configured = false;
		gamepad_usb_publish(USB_STATE_CONNECTED);
		break;
	case USBD_MSG_CONFIGURATION:
		/* The status is the configuration value, 0 when the host removed it. */
		configured = (msg->status != 0);
		gamepad_usb_publish(configured ? USB_STATE_CONFIGURED : USB_STATE_CONNECTED);
		break;
	case USBD_MSG_SUSPEND:
		gamepad_usb_publish(USB_STATE_SUSPENDED);
		break;
	case USBD_MSG_RESUME:
		gamepad_usb_publish(configured ? USB_STATE_CONFIGURED : USB_STATE_CONNECTED);
		break;
	default:
		break;
	}
}

int gamepad_usb_init(void)
{
	int ret;

	ret = usbd_add_descriptor(&gamepad_usbd, &gamepad_lang);
	if (ret != 0) {
		LOG_ERR("Failed to add the language descriptor (%d)", ret);
		return ret;
	}

	ret = usbd_add_descriptor(&gamepad_usbd, &gamepad_manufacturer);
	if (ret != 0) {
		LOG_ERR("Failed to add the manufacturer descriptor (%d)", ret);
		return ret;
	}

	ret = usbd_add_descriptor(&gamepad_usbd, &gamepad_product);
	if (ret != 0) {
		LOG_ERR("Failed to add the product descriptor (%d)", ret);
		return ret;
	}

	ret = usbd_add_descriptor(&gamepad_usbd, &gamepad_serial);
	if (ret != 0) {
		LOG_ERR("Failed to add the serial number descriptor (%d)", ret);
		return ret;
	}

	ret = usbd_add_configuration(&gamepad_usbd, USBD_SPEED_FS, &gamepad_fs_config);
	if (ret != 0) {
		LOG_ERR("Failed to add the configuration (%d)", ret);
		return ret;
	}

	ret = usbd_register_all_classes(&gamepad_usbd, USBD_SPEED_FS, 1, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to register the USB classes (%d)", ret);
		return ret;
	}

	ret = usbd_device_set_bcd_device(&gamepad_usbd, GAMEPAD_USB_BCD_VERSION);
	if (ret != 0) {
		LOG_ERR("Failed to set the device release number (%d)", ret);
		return ret;
	}

	/* The class, subclass and protocol are given by the interface descriptors. */
	usbd_device_set_code_triple(&gamepad_usbd, USBD_SPEED_FS, 0, 0, 0);
	usbd_self_powered(&gamepad_usbd, false);

	ret = usbd_msg_register_cb(&gamepad_usbd, gamepad_usb_msg_cb);
	if (ret != 0) {
		LOG_ERR("Failed to register the message callback (%d)", ret);
		return ret;
	}

	ret = usbd_init(&gamepad_usbd);
	if (ret != 0) {
		LOG_ERR("Failed to initialize the USB device (%d)", ret);
		return ret;
	}

	/* A controller that detects VBUS is enabled when VBUS is there, see gamepad_usb_msg_cb().
	 */
	if (!usbd_can_detect_vbus(&gamepad_usbd)) {
		ret = usbd_enable(&gamepad_usbd);
		if (ret != 0) {
			LOG_ERR("Failed to enable the USB device (%d)", ret);
			return ret;
		}

		gamepad_usb_publish(USB_STATE_CONNECTED);
	}

	return 0;
}

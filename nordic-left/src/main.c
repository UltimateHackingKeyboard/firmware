#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>


#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <assert.h>
#include <zephyr/spinlock.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/bluetooth/services/bas.h>
#include <bluetooth/services/hids.h>
#include <zephyr/bluetooth/services/dis.h>
#include <dk_buttons_and_leds.h>

static const uint8_t hid_keyboard_report_desc[] = HID_KEYBOARD_REPORT_DESC();
static const uint8_t hid_mouse_report_desc[] = HID_MOUSE_REPORT_DESC(2);

static enum usb_dc_status_code usb_status;

#define MOUSE_BTN_REPORT_POS	0
#define MOUSE_X_REPORT_POS	1
#define MOUSE_Y_REPORT_POS	2

#define MOUSE_BTN_LEFT		BIT(0)
#define MOUSE_BTN_RIGHT		BIT(1)
#define MOUSE_BTN_MIDDLE	BIT(2)

uint8_t mouse_report[4] = { 0x00 };
uint8_t keyboard_report[8] = { 0x00 };
const struct device *hid_keyboard_dev;
const struct device *hid_mouse_dev;

static void status_cb(enum usb_dc_status_code status, const uint8_t *param) {
	printk("USB status code change: %i\n", status);
	usb_status = status;
	if (status == USB_DC_CONFIGURED) {
		hid_int_ep_write(hid_mouse_dev, mouse_report, sizeof(mouse_report), NULL);
		hid_int_ep_write(hid_keyboard_dev, keyboard_report, sizeof(keyboard_report), NULL);
	}
}

/*
	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		if (usb_status == USB_DC_SUSPEND) {
			usb_wakeup_request();
			return;
		}
	}
*/

void int_in_ready_mouse(const struct device *dev) {
	uint32_t buttons = dk_get_buttons();
	mouse_report[MOUSE_X_REPORT_POS] = buttons & DK_BTN1_MSK ? 5 : 0;
	mouse_report[MOUSE_Y_REPORT_POS] = buttons & DK_BTN2_MSK ? 5 : 0;
	mouse_report[MOUSE_BTN_REPORT_POS] = buttons & DK_BTN3_MSK ? MOUSE_BTN_LEFT : 0;
	hid_int_ep_write(hid_mouse_dev, mouse_report, sizeof(mouse_report), NULL);
}

void int_in_ready_keyboard(const struct device *dev) {
	uint32_t buttons = dk_get_buttons();
	keyboard_report[2] = buttons & DK_BTN4_MSK ? HID_KEY_A : 0;
	hid_int_ep_write(hid_keyboard_dev, keyboard_report, sizeof(keyboard_report), NULL);
}

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define BASE_USB_HID_SPEC_VERSION   0x0101

#define OUTPUT_REPORT_MAX_LEN            1
#define OUTPUT_REPORT_BIT_MASK_CAPS_LOCK 0x02
#define INPUT_REP_KEYS_REF_ID            1
#define OUTPUT_REP_KEYS_REF_ID           1
#define SCAN_CODE_POS                    2
#define KEYS_MAX_LEN                    (INPUT_REPORT_KEYS_MAX_LEN - SCAN_CODE_POS)

#define ADV_LED_BLINK_INTERVAL  1000

#define ADV_STATUS_LED DK_LED1
#define CON_STATUS_LED DK_LED2
#define LED_CAPS_LOCK  DK_LED3
#define KEY_TEXT_MASK  DK_BTN1_MSK

// Key used to accept or reject passkey value
#define KEY_PAIRING_ACCEPT DK_BTN1_MSK
#define KEY_PAIRING_REJECT DK_BTN2_MSK

static volatile bool is_adv;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
		      (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
		      (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
					  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct {
	struct bt_conn *conn;
	bool in_boot_mode;
} conn_mode;

struct {
	struct bt_conn *conn;
	unsigned int passkey;
} pairing_data;

static void advertising_start(void) {
	struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
						BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME,
						BT_GAP_ADV_FAST_INT_MIN_2,
						BT_GAP_ADV_FAST_INT_MAX_2,
						NULL);

	int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		if (err == -EALREADY) {
			printk("Advertising continued\n");
		} else {
			printk("Advertising failed to start (err %d)\n", err);
		}

		return;
	}

	is_adv = true;
	printk("Advertising successfully started\n");
}

static void pairing_process() {
	if (!pairing_data.conn) {
		return;
	}

	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(pairing_data.conn), addr, sizeof(addr));
	printk("Passkey for %s: %06u\n", addr, pairing_data.passkey);
	printk("Press Button 1 to confirm, Button 2 to reject.\n");
}

// HID init

static void caps_lock_handler(const struct bt_hids_rep *rep) {
	uint8_t report_val = ((*rep->data) & OUTPUT_REPORT_BIT_MASK_CAPS_LOCK) ? 1 : 0;
	dk_set_led(LED_CAPS_LOCK, report_val);
}

static void hids_outp_rep_handler(struct bt_hids_rep *rep, struct bt_conn *conn, bool write) {
	char addr[BT_ADDR_LE_STR_LEN];

	if (!write) {
		printk("Output report read\n");
		return;
	};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Output report has been received %s\n", addr);
	caps_lock_handler(rep);
}

static void hids_boot_kb_outp_rep_handler(struct bt_hids_rep *rep, struct bt_conn *conn, bool write) {
	char addr[BT_ADDR_LE_STR_LEN];

	if (!write) {
		printk("Output report read\n");
		return;
	};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Boot Keyboard Output report has been received %s\n", addr);
	caps_lock_handler(rep);
}

static void hids_pm_evt_handler(enum bt_hids_pm_evt evt, struct bt_conn *conn) {
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn_mode.conn != conn) {
		printk("Cannot find connection handle when processing PM");
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	switch (evt) {
	case BT_HIDS_PM_EVT_BOOT_MODE_ENTERED:
		printk("Boot mode entered %s\n", addr);
		conn_mode.in_boot_mode = true;
		break;
	case BT_HIDS_PM_EVT_REPORT_MODE_ENTERED:
		printk("Report mode entered %s\n", addr);
		conn_mode.in_boot_mode = false;
		break;
	default:
		break;
	}
}

// Bluetooth keyboard

// Note: The configuration below is the same as BOOT mode configuration
// This simplifies the code as the BOOT mode is the same as REPORT mode.
// Changing this configuration would require separate implementation of
// BOOT mode report generation.
#define KEY_PRESS_MAX 6 // Maximum number of non-control keys pressed simultaneously

// Number of bytes in key report
//
// 1 byte: modifier keys
// 1 byte: reserved
// rest: non-control keys
#define INPUT_REPORT_KEYS_MAX_LEN (1 + 1 + KEY_PRESS_MAX)

// INPUT report internal indexes.
// This is a position in internal report table and is not related to report ID.
enum {
	INPUT_REP_KEYS_IDX = 0
};

BT_HIDS_DEF(hids_keyboard_obj,
	    OUTPUT_REPORT_MAX_LEN,
	    INPUT_REPORT_KEYS_MAX_LEN);

static void hid_keyboard_init(void) {
	static const uint8_t report_map[] = {
		0x05, 0x01,       /* Usage Page (Generic Desktop) */
		0x09, 0x06,       /* Usage (Keyboard) */
		0xA1, 0x01,       /* Collection (Application) */

		/* Keys */
#if INPUT_REP_KEYS_REF_ID
		0x85, INPUT_REP_KEYS_REF_ID,
#endif
		0x05, 0x07,       /* Usage Page (Key Codes) */
		0x19, 0xe0,       /* Usage Minimum (224) */
		0x29, 0xe7,       /* Usage Maximum (231) */
		0x15, 0x00,       /* Logical Minimum (0) */
		0x25, 0x01,       /* Logical Maximum (1) */
		0x75, 0x01,       /* Report Size (1) */
		0x95, 0x08,       /* Report Count (8) */
		0x81, 0x02,       /* Input (Data, Variable, Absolute) */

		0x95, 0x01,       /* Report Count (1) */
		0x75, 0x08,       /* Report Size (8) */
		0x81, 0x01,       /* Input (Constant) reserved byte(1) */

		0x95, 0x06,       /* Report Count (6) */
		0x75, 0x08,       /* Report Size (8) */
		0x15, 0x00,       /* Logical Minimum (0) */
		0x25, 0x65,       /* Logical Maximum (101) */
		0x05, 0x07,       /* Usage Page (Key codes) */
		0x19, 0x00,       /* Usage Minimum (0) */
		0x29, 0x65,       /* Usage Maximum (101) */
		0x81, 0x00,       /* Input (Data, Array) Key array(6 bytes) */

		/* LED */
#if OUTPUT_REP_KEYS_REF_ID
		0x85, OUTPUT_REP_KEYS_REF_ID,
#endif
		0x95, 0x05,       /* Report Count (5) */
		0x75, 0x01,       /* Report Size (1) */
		0x05, 0x08,       /* Usage Page (Page# for LEDs) */
		0x19, 0x01,       /* Usage Minimum (1) */
		0x29, 0x05,       /* Usage Maximum (5) */
		0x91, 0x02,       /* Output (Data, Variable, Absolute), */
   	    /* Led report */
		0x95, 0x01,       /* Report Count (1) */
		0x75, 0x03,       /* Report Size (3) */
		0x91, 0x01,       /* Output (Data, Variable, Absolute), */
		/* Led report padding */

		0xC0              /* End Collection (Application) */
	};

	struct bt_hids_init_param hids_init_obj = {
		.rep_map.data = report_map,
		.rep_map.size = sizeof(report_map),
		.info = {
			.bcd_hid = BASE_USB_HID_SPEC_VERSION,
			.b_country_code = 0x00,
			.flags = BT_HIDS_REMOTE_WAKE | BT_HIDS_NORMALLY_CONNECTABLE,
		},
		.inp_rep_group_init = {
			.reports = {
				{
					.size = INPUT_REPORT_KEYS_MAX_LEN,
					.id = INPUT_REP_KEYS_REF_ID,
				}
			},
			.cnt = 1,
		},
		.outp_rep_group_init = {
			.reports = {
				{
					.size = OUTPUT_REPORT_MAX_LEN,
					.id = OUTPUT_REP_KEYS_REF_ID,
					.handler = hids_outp_rep_handler,
				}
			},
			.cnt =  1,
		},
		.is_kb = true,
		.boot_kb_outp_rep_handler = hids_boot_kb_outp_rep_handler,
		.pm_evt_handler = hids_pm_evt_handler,
	};

	int err = bt_hids_init(&hids_keyboard_obj, &hids_init_obj);
	__ASSERT(err == 0, "HIDS keyboard initialization failed\n");
}

// Bluetooth mouse

// Length of Mouse Input Report containing button data.
#define INPUT_REP_BUTTONS_LEN       3
// Length of Mouse Input Report containing movement data.
#define INPUT_REP_MOVEMENT_LEN      3
// Length of Mouse Input Report containing media player data.
#define INPUT_REP_MEDIA_PLAYER_LEN  1
// Id of reference to Mouse Input Report containing button data.
#define INPUT_REP_REF_BUTTONS_ID    1
// Id of reference to Mouse Input Report containing movement data.
#define INPUT_REP_REF_MOVEMENT_ID   2
// Id of reference to Mouse Input Report containing media player data.
#define INPUT_REP_REF_MPLAYER_ID    3

BT_HIDS_DEF(hids_mouse_obj,
	    INPUT_REP_BUTTONS_LEN,
	    INPUT_REP_MOVEMENT_LEN,
	    INPUT_REP_MEDIA_PLAYER_LEN);

static void hid_mouse_init(void)
{
	static const uint8_t report_map[] = {
		0x05, 0x01,     // Usage Page (Generic Desktop)
		0x09, 0x02,     // Usage (Mouse)

		0xA1, 0x01,     // Collection (Application)

		// Report ID 1: Mouse buttons + scroll/pan
		0x85, 0x01,       // Report Id 1
		0x09, 0x01,       // Usage (Pointer)
		0xA1, 0x00,       // Collection (Physical)
		0x95, 0x05,       // Report Count (3)
		0x75, 0x01,       // Report Size (1)
		0x05, 0x09,       // Usage Page (Buttons)
		0x19, 0x01,       // Usage Minimum (01)
		0x29, 0x05,       // Usage Maximum (05)
		0x15, 0x00,       // Logical Minimum (0)
		0x25, 0x01,       // Logical Maximum (1)
		0x81, 0x02,       // Input (Data, Variable, Absolute)
		0x95, 0x01,       // Report Count (1)
		0x75, 0x03,       // Report Size (3)
		0x81, 0x01,       // Input (Constant) for padding
		0x75, 0x08,       // Report Size (8)
		0x95, 0x01,       // Report Count (1)
		0x05, 0x01,       // Usage Page (Generic Desktop)
		0x09, 0x38,       // Usage (Wheel)
		0x15, 0x81,       // Logical Minimum (-127)
		0x25, 0x7F,       // Logical Maximum (127)
		0x81, 0x06,       // Input (Data, Variable, Relative)
		0x05, 0x0C,       // Usage Page (Consumer)
		0x0A, 0x38, 0x02, // Usage (AC Pan)
		0x95, 0x01,       // Report Count (1)
		0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
		0xC0,             // End Collection (Physical)

		// Report ID 2: Mouse motion
		0x85, 0x02,       // Report Id 2
		0x09, 0x01,       // Usage (Pointer)
		0xA1, 0x00,       // Collection (Physical)
		0x75, 0x0C,       // Report Size (12)
		0x95, 0x02,       // Report Count (2)
		0x05, 0x01,       // Usage Page (Generic Desktop)
		0x09, 0x30,       // Usage (X)
		0x09, 0x31,       // Usage (Y)
		0x16, 0x01, 0xF8, // Logical maximum (2047)
		0x26, 0xFF, 0x07, // Logical minimum (-2047)
		0x81, 0x06,       // Input (Data, Variable, Relative)
		0xC0,             // End Collection (Physical)
		0xC0,             // End Collection (Application)

		// Report ID 3: Advanced buttons
		0x05, 0x0C,       // Usage Page (Consumer)
		0x09, 0x01,       // Usage (Consumer Control)
		0xA1, 0x01,       // Collection (Application)
		0x85, 0x03,       // Report Id (3)
		0x15, 0x00,       // Logical minimum (0)
		0x25, 0x01,       // Logical maximum (1)
		0x75, 0x01,       // Report Size (1)
		0x95, 0x01,       // Report Count (1)

		0x09, 0xCD,       // Usage (Play/Pause)
		0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
		0x0A, 0x83, 0x01, // Usage (Consumer Control Configuration)
		0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
		0x09, 0xB5,       // Usage (Scan Next Track)
		0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
		0x09, 0xB6,       // Usage (Scan Previous Track)
		0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)

		0x09, 0xEA,       // Usage (Volume Down)
		0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
		0x09, 0xE9,       // Usage (Volume Up)
		0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
		0x0A, 0x25, 0x02, // Usage (AC Forward)
		0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
		0x0A, 0x24, 0x02, // Usage (AC Back)
		0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
		0xC0              // End Collection
	};

	struct bt_hids_init_param hids_init_param = { 0 };
	hids_init_param.rep_map.data = report_map;
	hids_init_param.rep_map.size = sizeof(report_map);

	hids_init_param.info.bcd_hid = BASE_USB_HID_SPEC_VERSION;
	hids_init_param.info.b_country_code = 0x00;
	hids_init_param.info.flags = BT_HIDS_REMOTE_WAKE | BT_HIDS_NORMALLY_CONNECTABLE;

	struct bt_hids_inp_rep *hids_inp_rep;
	hids_inp_rep = &hids_init_param.inp_rep_group_init.reports[0];
	hids_inp_rep->size = INPUT_REP_BUTTONS_LEN;
	hids_inp_rep->id = INPUT_REP_REF_BUTTONS_ID;
	hids_init_param.inp_rep_group_init.cnt++;

	static const uint8_t mouse_movement_mask[ceiling_fraction(INPUT_REP_MOVEMENT_LEN, 8)] = {0};
	hids_inp_rep++;
	hids_inp_rep->size = INPUT_REP_MOVEMENT_LEN;
	hids_inp_rep->id = INPUT_REP_REF_MOVEMENT_ID;
	hids_inp_rep->rep_mask = mouse_movement_mask;
	hids_init_param.inp_rep_group_init.cnt++;

	hids_inp_rep++;
	hids_inp_rep->size = INPUT_REP_MEDIA_PLAYER_LEN;
	hids_inp_rep->id = INPUT_REP_REF_MPLAYER_ID;
	hids_init_param.inp_rep_group_init.cnt++;

	hids_init_param.is_mouse = true;
	hids_init_param.pm_evt_handler = hids_pm_evt_handler;

	int err = bt_hids_init(&hids_mouse_obj, &hids_init_param);
	__ASSERT(err == 0, "HIDS mouse initialization failed\n");
}

// Connection callbacks

static void connected(struct bt_conn *conn, uint8_t err) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected %s\n", addr);
	dk_set_led_on(CON_STATUS_LED);

	err = bt_hids_connected(&hids_keyboard_obj, conn);

	if (err) {
		printk("Failed to notify HID service about connection\n");
		return;
	}

	if (!conn_mode.conn) {
		conn_mode.conn = conn;
		conn_mode.in_boot_mode = false;
		advertising_start();
	}

	is_adv = false;
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Disconnected from %s (reason %u)\n", addr, reason);

	int err = bt_hids_disconnected(&hids_keyboard_obj, conn);
	if (err) {
		printk("Failed to notify HID service about disconnection\n");
	}

	conn_mode.conn = NULL;
	dk_set_led_off(CON_STATUS_LED);
	advertising_start();
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d\n", addr, level, err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

// Auth callbacks

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey) {
	pairing_data.conn    = bt_conn_ref(conn);
	pairing_data.passkey = passkey;
	pairing_process();
}

static void auth_cancel(struct bt_conn *conn) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_display = auth_passkey_display,
	.passkey_confirm = auth_passkey_confirm,
	.cancel = auth_cancel,
};

// Auth info callbacks

static void pairing_complete(struct bt_conn *conn, bool bonded) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason) {
	if (!pairing_data.conn) {
		return;
	}

	if (pairing_data.conn == conn) {
		bt_conn_unref(pairing_data.conn);
		pairing_data.conn = NULL;
	}

	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Pairing failed conn: %s, reason %d\n", addr, reason);
}

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

static int key_report_send(bool down) {
	if (!conn_mode.conn) {
		return 0;
	}

	uint8_t chr = down ? HID_KEY_A : 0;
	uint8_t data[INPUT_REPORT_KEYS_MAX_LEN] = {0 /* modifiers */, 0, chr, 0, 0, 0, 0, 0};

	int err = 0;
	if (conn_mode.in_boot_mode) {
		err = bt_hids_boot_kb_inp_rep_send(&hids_keyboard_obj, conn_mode.conn, data, sizeof(data), NULL);
	} else {
		err = bt_hids_inp_rep_send(&hids_keyboard_obj, conn_mode.conn, INPUT_REP_KEYS_IDX, data, sizeof(data), NULL);
	}

	if (err) {
		printk("Key report send error: %d\n", err);
	}

	return err;
}

static void num_comp_reply(bool accept) {
	struct bt_conn *conn;

	if (!pairing_data.conn) {
		return;
	}

	conn = pairing_data.conn;

	if (accept) {
		bt_conn_auth_passkey_confirm(conn);
		printk("Numeric Match, conn %p\n", conn);
	} else {
		bt_conn_auth_cancel(conn);
		printk("Numeric Reject, conn %p\n", conn);
	}

	bt_conn_unref(pairing_data.conn);
	pairing_data.conn = NULL;
}

static void button_changed(uint32_t button_state, uint32_t has_changed) {
	static bool pairing_button_pressed;

	uint32_t buttons = button_state & has_changed;

	if (pairing_data.conn) {
		if (buttons & KEY_PAIRING_ACCEPT) {
			pairing_button_pressed = true;
			num_comp_reply(true);
			return;
		}

		if (buttons & KEY_PAIRING_REJECT) {
			pairing_button_pressed = true;
			num_comp_reply(false);
			return;
		}
	}

	/* Do not take any action if the pairing button is released. */
	if (pairing_button_pressed && (has_changed & (KEY_PAIRING_ACCEPT | KEY_PAIRING_REJECT))) {
		pairing_button_pressed = false;
		return;
	}

	if (has_changed & KEY_TEXT_MASK) {
		key_report_send((button_state & KEY_TEXT_MASK) != 0);
	}
}

static void bas_notify(void) {
	uint8_t battery_level = bt_bas_get_battery_level();

	battery_level--;

	if (!battery_level) {
		battery_level = 100U;
	}

	bt_bas_set_battery_level(battery_level);
}

void main(void) {
	// USB

	struct hid_ops hidops_mouse = {
		.int_in_ready = int_in_ready_mouse,
	};

	struct hid_ops hidops_keyboard = {
		.int_in_ready = int_in_ready_keyboard,
	};

	hid_keyboard_dev = device_get_binding("HID_0");
	hid_mouse_dev = device_get_binding("HID_1");

	dk_buttons_init(button_changed);
	dk_leds_init();

	usb_hid_register_device(hid_keyboard_dev, hid_keyboard_report_desc, sizeof(hid_keyboard_report_desc), &hidops_keyboard);
	usb_hid_register_device(hid_mouse_dev, hid_mouse_report_desc, sizeof(hid_mouse_report_desc), &hidops_mouse);

	usb_hid_init(hid_keyboard_dev);
	usb_hid_init(hid_mouse_dev);

	usb_enable(status_cb);

	// Bluetooth

	int blink_status = 0;

	printk("Starting Bluetooth Peripheral HIDS keyboard example\n");

	bt_conn_auth_cb_register(&conn_auth_callbacks);
	bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	bt_enable(NULL);

	printk("Bluetooth initialized\n");

	hid_keyboard_init();
	hid_mouse_init();

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	advertising_start();

	for (;;) {
		if (is_adv) {
			dk_set_led(ADV_STATUS_LED, (++blink_status) % 2);
		} else {
			dk_set_led_off(ADV_STATUS_LED);
		}

		k_sleep(K_MSEC(ADV_LED_BLINK_INTERVAL));
		/* Battery level simulation */
		bas_notify();
	}
}

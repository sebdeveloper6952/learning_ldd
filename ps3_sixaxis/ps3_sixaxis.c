#include <linux/device.h>
#include <linux/hid.h>
#include <linux/module.h>
#include <linux/leds.h>
#include <linux/spinlock.h>

#define VENDOR_ID 0x054c
#define PRODUCT_ID 0x0268
#define LED_AMOUNT 4
#define SIXAXIS_CONTROLLER_USB    BIT(1)
#define SIXAXIS_CONTROLLER_BT     BIT(2)
#define SIXAXIS_CONTROLLER (SIXAXIS_CONTROLLER_USB | SIXAXIS_CONTROLLER_BT)
#define info(s, ...) printk(KERN_INFO s, ##__VA_ARGS__)

struct ps3_sixaxis_dev {
    spinlock_t lock;
    struct hid_device *hid_dev;
    struct led_classdev *leds[LED_AMOUNT];
    struct work_struct hotplug_worker;
	struct work_struct state_worker;
    u8 led_state[LED_AMOUNT];
	u8 led_delay_on[LED_AMOUNT];
	u8 led_delay_off[LED_AMOUNT];
	u8 led_count;
    u8 hotplug_worker_initialized;
	u8 state_worker_initialized;
	u8 defer_initialization;
};

enum sony_worker {
	SONY_WORKER_STATE,
	SONY_WORKER_HOTPLUG
};

static inline void sony_schedule_work(struct ps3_sixaxis_dev *dev,
				      enum sony_worker which)
{
	unsigned long flags;

	switch (which) {
	case SONY_WORKER_STATE:
		spin_lock_irqsave(&dev->lock, flags);
		if (!dev->defer_initialization && dev->state_worker_initialized)
			schedule_work(&dev->state_worker);
		spin_unlock_irqrestore(&dev->lock, flags);
		break;
	case SONY_WORKER_HOTPLUG:
		if (dev->hotplug_worker_initialized)
			schedule_work(&dev->hotplug_worker);
		break;
	}
}

static void set_leds(struct ps3_sixaxis_dev *dev)
{
    sony_schedule_work(dev, SONY_WORKER_STATE);
}

static int probe_fn(struct hid_device *hdev, const struct hid_device_id *id)
{
    int return_val;
    unsigned int connect_mask = HID_CONNECT_DEFAULT | HID_CONNECT_HIDDEV_FORCE;
    struct ps3_sixaxis_dev *dev;

    // managed allocation, is freed automatically and initialized to zero
    dev = devm_kzalloc(&hdev->dev, sizeof(struct ps3_sixaxis_dev), GFP_KERNEL);
    if (dev == NULL)
    {
        // couldnt allocate memory
        info("[ps3_sixaxis_driver] probe_fn: could not allocate memory.\n");
        return -ENOMEM;
    }

    //
    hid_set_drvdata(hdev, dev);
    dev->hid_dev = hdev;

    return_val = hid_parse(hdev);
    if (return_val)
    {
        // parse failed
        info("[ps3_sixaxis_driver] probe_fn: parse failed.\n");
        return return_val;
    }

    //
    return_val = hid_hw_start(hdev, connect_mask);

    info("[ps3_sixaxis_driver] probe_fn finished.\n");
    
    return return_val;
}

static void remove_fn(struct hid_device *hdev)
{
    //
    hid_hw_close(hdev);
    
    //
    hid_hw_stop(hdev);

    info("[ps3_sixaxis_driver] remove_fn finished.\n");
}

static int input_configured_fn(struct hid_device *hdev,
					struct hid_input *hidinput)
{
    info("[ps3_sixaxis_driver] input_configured_fn\n");

    // info("[ps3_sixaxis_driver] probe_fn setting leds.\n");
    // static const u8 sixaxis_leds[10][4] = {
	// 			{ 0x01, 0x00, 0x00, 0x00 },
	// 			{ 0x00, 0x01, 0x00, 0x00 },
	// 			{ 0x00, 0x00, 0x01, 0x00 },
	// 			{ 0x00, 0x00, 0x00, 0x01 },
	// 			{ 0x01, 0x00, 0x00, 0x01 },
	// 			{ 0x00, 0x01, 0x00, 0x01 },
	// 			{ 0x00, 0x00, 0x01, 0x01 },
	// 			{ 0x01, 0x00, 0x01, 0x01 },
	// 			{ 0x00, 0x01, 0x01, 0x01 },
	// 			{ 0x01, 0x01, 0x01, 0x01 }
	// };

	// BUILD_BUG_ON(LED_AMOUNT < ARRAY_SIZE(sixaxis_leds[0]));

	// memcpy(dev->led_state, sixaxis_leds[0], sizeof(sixaxis_leds[0]));

    return 0;
}

static const struct hid_device_id device_id_table[] = {
    { HID_USB_DEVICE(VENDOR_ID, PRODUCT_ID), .driver_data = BIT(1) },
    {}
};
MODULE_DEVICE_TABLE(hid, device_id_table);

static struct hid_driver ps3_sixaxis_driver = {
	.name             = "sebdev6992_ps3_sixaxis",
	.id_table         = device_id_table,
	.probe            = probe_fn,
	.remove           = remove_fn,
    .input_configured = input_configured_fn,
};

static int __init ps3_sixaxis_init(void) 
{
    info("[ps3_sixaxis_driver] driver registration.\n");
    return hid_register_driver(&ps3_sixaxis_driver);
};

static void __exit ps3_sixaxis_exit(void) 
{
    hid_unregister_driver(&ps3_sixaxis_driver);
    info("[ps3_sixaxis_driver] driver unregistered.\n");
};

module_init(ps3_sixaxis_init);
module_exit(ps3_sixaxis_exit);

MODULE_LICENSE("GPL");
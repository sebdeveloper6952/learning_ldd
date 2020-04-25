#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/usb.h>
#include <asm/uaccess.h>

#define VENDOR_ID 0x951
#define PRODUCT_ID 0x1666
#define KINGSTON_USB_MINOR_BASE 192
#define info(s, ...) printk(KERN_INFO s, ##__VA_ARGS__)

/*
* device_table: defines the devices that this driver controls. 
* USB_DEVICE (macro): creates a struct usb_device_id that has vendor id
* and product id set.
* MODULE_DEVICE_TABLE (macro): 
*/
struct usb_device_id device_table[] = {
    {USB_DEVICE(VENDOR_ID, PRODUCT_ID)},
    {},
};
MODULE_DEVICE_TABLE(usb, device_table);

/* Structure to hold all of our device specific stuff */
struct kingston_usb_dev
{
    struct usb_device *udev;         /* the usb device for this device */
    struct usb_interface *interface; /* the interface for this device */
    unsigned char *bulk_in_buffer;   /* the buffer to receive data */
    size_t bulk_in_size;             /* the size of the receive buffer */
    __u8 bulk_in_endpoint_addr;      /* the address of the bulk in endpoint */
    __u8 bulk_out_endpoint_addr;     /* the address of the bulk out endpoint */
    struct kref kref;
};
#define get_kingston_dev_container(d) container_of(d, struct kingston_usb_dev, kref)

/*
 *
 */
static void
delete_fn(struct kref *kref)
{
    struct kingston_usb_dev *dev = get_kingston_dev_container(kref);
    usb_put_dev(dev->udev);
    kfree(dev->bulk_in_buffer);
    kfree(dev);
}

// just for convenience
static struct usb_driver kingston_usb_driver;

/*
 *
 */
static int open_fn(struct inode *inode, struct file *file)
{
    struct kingston_usb_dev *dev;
    struct usb_interface *intf;
    int subminor;
    int retval = 0;

    info("[sebdev6992] Kingston USB Driver (open_fn):\t START");

    // get subminor number from the inode
    subminor = iminor(inode);

    // a
    intf = usb_find_interface(&kingston_usb_driver, subminor);
    if (!intf)
    {
        info("[sebdev6992] Kingston USB Driver %s: error, cant find device for minor %d",
             __FUNCTION__, subminor);

        return -ENODEV;
    }

    dev = usb_get_intfdata(intf);
    if (!dev)
    {
        info("[sebdev6992] Kingston USB Driver %s: error, cant find device from interface.",
             __FUNCTION__);

        return -ENODEV;
    }

    // increment our reference count for our device
    kref_get(&dev->kref);

    // save our device in the file private data
    file->private_data = dev;

    info("[sebdev6992] Kingston USB Driver (open_fn):\t FINISH");
    return retval;
}

/*
 *
 */
struct file_operations fops = {
    .owner = THIS_MODULE,
    //.read = read_fn,
    //.write = write_fn,
    .open = open_fn,
    //.release = release_fn,
};

/* 
 * Defines some parameters that this driver wants the USB Core to know
 * when registering for a minor number.
 * @name: the usb class device name for this driver.  Will show up in sysfs.
 * @fops: pointer to the struct file_operations of this driver.
 * @minor_base: the start of the minor range for this driver.
 */
static struct usb_class_driver kingston_usb_class_driver = {
    .name = "sebdev6992_kingston_class_driver",
    .fops = &fops,
    .minor_base = KINGSTON_USB_MINOR_BASE,
};

/*
 * This function is called by the USB Core when a usb_interface (intf) could
 * be handled by this driver.
 * return 0: indicates this driver wants to handle the specified usb device.
 */
static int probe_fn(struct usb_interface *intf, const struct usb_device_id *id)
{
    struct kingston_usb_dev *dev = NULL;
    struct usb_host_interface *intf_desc;
    struct usb_endpoint_descriptor *endpoint;
    size_t buffer_size;
    int i;
    int retval = -ENOMEM;

    // allocate kernel memory
    dev = kmalloc(sizeof(struct kingston_usb_dev), GFP_KERNEL);

    // check for memory allocation error
    if (dev == NULL)
    {
        info("[sebdev6992] Kingston USB Driver (probe_fn): out of memory.");
        goto error;
    }

    // fill our struct with zeros
    memset(dev, 0x00, sizeof(*dev));

    // Set the amount of refs of this device to 1
    kref_init(&dev->kref);

    // container_of() is used to obtain a pointer to the containing struct of intf->dev.parent,
    // which is a struct usb_device
    // 1. to_usb_device(intf->dev.parent)
    // 2. container_of(d, struct usb_device, dev)
    dev->udev = usb_get_dev(interface_to_usbdev(intf));
    dev->interface = intf;

    // set up the endpoint information
    // use only the first bulk-in and bulk-out endpoints
    intf_desc = intf->cur_altsetting;
    for (i = 0; i < intf_desc->desc.bNumEndpoints; ++i)
    {
        endpoint = &intf_desc->endpoint[i].desc;

        // we found a bulk in endpoint, save its addresses and data
        // in our kingston_usb_dev struct
        if (!dev->bulk_in_endpoint_addr &&
            (endpoint->bEndpointAddress & USB_DIR_IN) &&
            ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK))
        {
            buffer_size = endpoint->wMaxPacketSize;
            dev->bulk_in_size = buffer_size;
            dev->bulk_in_endpoint_addr = endpoint->bEndpointAddress;
            // allocate memory for out data buffer, and check for errors
            dev->bulk_in_buffer = kmalloc(buffer_size, GFP_KERNEL);
            if (!dev->bulk_in_buffer)
            {
                info("[sebdev6992] Kingston USB Driver (probe_fn): Could not allocate bulk_in_buffer");
                goto error;
            }
        }

        // we found a bulk out endpoint, save its address in our
        // kingston_usb_dev struct
        if (!dev->bulk_out_endpoint_addr &&
            !(endpoint->bEndpointAddress & USB_DIR_IN) &&
            ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK))
        {
            dev->bulk_out_endpoint_addr = endpoint->bEndpointAddress;
        }
    }

    // failed to get bulk in and/or bulk out endpoints
    if (!(dev->bulk_in_endpoint_addr && dev->bulk_out_endpoint_addr))
    {
        info("[sebdev6992] Kingston USB Driver (probe_fn): Could not find both bulk-in and bulk-out endpoints");
        goto error;
    }

    // this saves our kingston_usb_dev struct in this interface memory
    // so we can retrieve it easily in other functions
    usb_set_intfdata(intf, dev);

    // register this device with the USB Core
    retval = usb_register_dev(intf, &kingston_usb_class_driver);
    if (retval)
    {
        /* something prevented us from registering this driver */
        info("[sebdev6992] Kingston USB Driver (probe_fn): Not able to get a minor for this device.");
        usb_set_intfdata(intf, NULL);
        goto error;
    }

    /* let the user know what node this device is now attached to */
    info("[sebdev6992] Kingston USB Driver (probe_fn): device now attached with minor %d", intf->minor);
    info("[sebdev6992] \tBulkIn EndpointAddr: %d", dev->bulk_in_endpoint_addr);
    info("[sebdev6992] \tBulkOut EndpointAddr: %d", dev->bulk_out_endpoint_addr);
    info("[sebdev6992]\tBuffer Size: %ld", dev->bulk_in_size);
    return 0;

error:
    if (dev)
    {
        // kref_put(&dev->kref, skel_delete);
        info("[sebdev6992] Kingston USB Driver (probe_fn): error.");
    }

    info("[sebdev6992] Kingston USB Driver (probe_fn): return value: %d", retval);
    return retval;
}

/*
*
*/
static void disconnect_fn(struct usb_interface *intf)
{
    struct kingston_usb_dev *dev;
    int minor = intf->minor;

    // TODO: replace as they are deprecated
    // lock_kernel();

    //
    dev = usb_get_intfdata(intf);
    usb_set_intfdata(intf, NULL);

    //
    usb_deregister_dev(intf, &kingston_usb_class_driver);

    // unlock_kernel();

    //
    kref_put(&dev->kref, delete_fn);

    info("[sebdev6992] Kingston USB Driver (disconnect_fn): disconnected minor: %d.", minor);
}

/*
*
*/
static struct usb_driver kingston_usb_driver = {
    .name = "sebdev6992_kingston_driver",
    .probe = probe_fn,
    .disconnect = disconnect_fn,
    .id_table = device_table,
};

/*
*
*/
static int __init kingston_init(void)
{
    int result;
    info("[sebdev6992] Kingston USB Driver (kingston_init): registering driver.");
    result = usb_register(&kingston_usb_driver);
    if (result)
        info("[sebdev6992] Kingston USB Driver (kingston_init): failed to register driver: %d", result);
    else
        info("[sebdev6992] Kingston USB Driver (kingston_init): driver registered.");
    return result;
}

/*
*
*/
static void __exit kingston_exit(void)
{
    info("[sebdev6992] Kingston USB Driver (kingston_exit): function called.");
    usb_deregister(&kingston_usb_driver);
}

module_init(kingston_init);
module_exit(kingston_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sebastian Arriola");
MODULE_DESCRIPTION("Prueba de USB Driver para Kingston Data Traveler");
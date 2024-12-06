#include <linux/delay.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/usb.h>

#define USB_ECHO_VENDOR_ID  0x08DE
#define USB_ECHO_PRODUCT_ID 0x0001

#define USB_ECHO_EP_IN  0x01
#define USB_ECHO_EP_OUT 0x02

#define PRINT_USB_INTERFACE_DESCRIPTOR(i)                                     \
{                                                                             \
    printk(KERN_EMERG "USB_INTERFACE_DESCRIPTOR:\n");                         \
    printk(KERN_EMERG "-----------------------------\n");                     \
    printk(KERN_EMERG "bLength: 0x%x\n", i.bLength);                          \
    printk(KERN_EMERG "bDescriptorType: 0x%x\n", i.bDescriptorType);          \
    printk(KERN_EMERG "bInterfaceNumber: 0x%x\n", i.bInterfaceNumber);        \
    printk(KERN_EMERG "bAlternateSetting: 0x%x\n", i.bAlternateSetting);      \
    printk(KERN_EMERG "bNumEndpoints: 0x%x\n", i.bNumEndpoints);              \
    printk(KERN_EMERG "bInterfaceClass: 0x%x\n", i.bInterfaceClass);          \
    printk(KERN_EMERG "bInterfaceSubClass: 0x%x\n", i.bInterfaceSubClass);    \
    printk(KERN_EMERG "bInterfaceProtocol: 0x%x\n", i.bInterfaceProtocol);    \
    printk(KERN_EMERG "iInterface: 0x%x\n", i.iInterface);                    \
    printk(KERN_EMERG "\n");                                                  \
}

#define PRINT_USB_ENDPOINT_DESCRIPTOR(e)                                      \
{                                                                             \
    printk(KERN_EMERG "USB_ENDPOINT_DESCRIPTOR:\n");                          \
    printk(KERN_EMERG "------------------------\n");                          \
    printk(KERN_EMERG "bLength: 0x%x\n", e.bLength);                          \
    printk(KERN_EMERG "bDescriptorType: 0x%x\n", e.bDescriptorType);          \
    printk(KERN_EMERG "bEndPointAddress: 0x%x\n", e.bEndpointAddress);        \
    printk(KERN_EMERG "bmAttributes: 0x%x\n", e.bmAttributes);                \
    printk(KERN_EMERG "wMaxPacketSize: 0x%x\n", e.wMaxPacketSize);            \
    printk(KERN_EMERG "bInterval: 0x%x\n", e.bInterval);                      \
    printk(KERN_EMERG "\n");                                                  \
}

static struct usb_device *dev;

static int usb_ipc_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    unsigned int endpoint_index;
    unsigned int endpoints_count;
    struct usb_host_interface *iface_desc = interface->cur_altsetting;

    dev = interface_to_usbdev(interface);
    printk(KERN_EMERG "USB Driver Probed: Vendor ID : 0x%04x,\tProduct ID : 0x%04x\n", id->idVendor, id->idProduct);
    endpoints_count = iface_desc->desc.bNumEndpoints;
    PRINT_USB_INTERFACE_DESCRIPTOR(iface_desc->desc);

    for (endpoint_index = 0; endpoint_index < endpoints_count; endpoint_index++ ) {
        PRINT_USB_ENDPOINT_DESCRIPTOR(iface_desc->endpoint[endpoint_index].desc);
    }

    return 0;
}

static void usb_ipc_disconnect(struct usb_interface *interface)
{
    (void)interface;
    printk(KERN_EMERG "USB IPC disconnect!\n");
}

const struct usb_device_id usb_ipc_table[] = {
    { USB_DEVICE( USB_ECHO_VENDOR_ID, USB_ECHO_PRODUCT_ID ) },
    { },
};

MODULE_DEVICE_TABLE(usb, usb_ipc_table);

static struct usb_driver usb_ipc_driver = {
    .name       = "USB IPC Driver",
    .probe      = usb_ipc_probe,
    .disconnect = usb_ipc_disconnect,
    .id_table   = usb_ipc_table,
};

static int __init usb_ipc_init(void)
{
    int err;

    err = usb_register(&usb_ipc_driver);
    if (err) {
        printk(KERN_EMERG "USB IPC init failed!\n");
        return err;
    }

    printk(KERN_EMERG "USB IPC init!\n");
    return 0;
}

static void __exit usb_ipc_exit(void)
{
    usb_deregister(&usb_ipc_driver);
    printk(KERN_EMERG "Test remove!\n");
}

static void usb_ipc_test_irq(struct urb *urb)
{
    if (!urb->actual_length) {
        usb_submit_urb(urb, GFP_KERNEL);
    } else {
        printk(KERN_EMERG "USB IPC test irq, buf: %x\n", *((uint8_t *)urb->transfer_buffer));
    }
}

static void usb_ipc_test_comp(struct urb *urb)
{
    (void)urb;
}

static int usb_ipc_test(const char *val, const struct kernel_param *kp)
{
    int err, data;
    struct urb *p_out, *p_in;
    void *buf_in;
    int8_t buf_out[1];

    (void)kp;

    if (!dev) {
        printk(KERN_EMERG "USB IPC test failed to get a device!\n");
        return -ENODEV;
    }

	if (!val || !strlen(val)) {
        printk(KERN_EMERG "USB IPC test failed with wrong value string!\n");
        return -EINVAL;
    }

    err = kstrtoint(val, 10, &data);
    if (err) {
        printk(KERN_EMERG "USB IPC test failed with wrong value!\n");
        return -EINVAL;
    }

    p_in = usb_alloc_urb(/* isoframes = */ 0, GFP_KERNEL);
    if (!p_in) {
        printk(KERN_EMERG "USB IPC test failed to allocate an in packet!\n");
        return -ENOMEM;
    }

    buf_in = usb_alloc_coherent(dev, 1, GFP_KERNEL, &p_in->transfer_dma);
    if (!buf_in) {
        printk(KERN_EMERG "USB IPC test failed to allocate an in buffer!\n");
        usb_free_urb(p_in);
        return -ENOMEM;
    }

    usb_fill_int_urb(p_in, dev, usb_rcvintpipe(dev, USB_ECHO_EP_IN), buf_in, sizeof(buf_in), usb_ipc_test_irq, NULL, 1);
    err = usb_submit_urb(p_in, GFP_KERNEL);
    if (err) {
        printk(KERN_EMERG "USB IPC test failed to submit an in packet, err: %d\n", err);
        usb_free_urb(p_in);
        return err;
    }

    p_out = usb_alloc_urb(/* isoframes = */ 0, GFP_KERNEL);
    if (!p_out) {
        printk(KERN_EMERG "USB IPC test failed to allocate an out packet!\n");
        return -ENOMEM;
    }

    buf_out[0] = data;
    usb_fill_int_urb(p_out, dev, usb_sndintpipe(dev, USB_ECHO_EP_OUT), buf_out, sizeof(buf_out), usb_ipc_test_comp, NULL, 1);
    err = usb_submit_urb(p_out, GFP_KERNEL);
    if (err) {
        printk(KERN_EMERG "USB IPC test failed to submit an out packet, err: %d\n", err);
        usb_free_urb(p_out);
        return err;
    }

    msleep(1000);

    return 0;
}

static const struct kernel_param_ops usb_ipc_test_ops = {
	.set = usb_ipc_test,
};

static int usb_ipc_test_param;
module_param_cb(test, &usb_ipc_test_ops, &usb_ipc_test_param, 0664);

module_init(usb_ipc_init);
module_exit(usb_ipc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dmytro Stechenko <dstechenko@meta.com>");

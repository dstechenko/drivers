#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/error-report.h"

#include "hw/usb.h"

#include "desc.h"

#define USB_ECHO_VENDOR_ID  0x08DE
#define USB_ECHO_PRODUCT_ID 0x0001

#define USB_ECHO_EP_IN  0x01
#define USB_ECHO_EP_OUT 0x02

typedef struct {
    USBDevice dev;
    uint8_t buf[64];
    size_t buf_len;
} USBEchoState;

enum {
    STR_MANUFACTURER = 1,
    STR_PRODUCT,
    STR_SERIAL,
    STR_CONFIG_SUPER,
};

static const USBDescStrings usb_echo_desc_strings = {
    [STR_MANUFACTURER] = "META",
    [STR_PRODUCT]      = "META USB ECHO",
    [STR_SERIAL]       = "0x42",
    [STR_CONFIG_SUPER] = "Super speed config (usb 3.0)",
};

static void usb_echo_handle_control(USBDevice *dev, USBPacket *p, int request, int value, int index, int length, uint8_t *data) {
    int ret;

    ret = usb_desc_handle_control(dev, p, request, value, index, length, data);
    if (ret >= 0) {
        return;
    }

    p->status = USB_RET_STALL;
    error_report("usb-echo: handle control unexpected packet");
}

static int8_t buf[1]= {0};
static bool buf_ready = false;

static void usb_echo_handle_data(USBDevice *dev, USBPacket *p) {
    USBEchoState *s = DO_UPCAST(USBEchoState, dev, dev);

    if (p->ep->nr == USB_ECHO_EP_IN && buf_ready) {
        usb_packet_copy(p, buf, sizeof(buf));
        buf_ready = false;
        error_report("usb-echo: handle data in, buf: %x", buf[0]);
    } else if (p->ep->nr == USB_ECHO_EP_OUT) {
        usb_packet_copy(p, buf, sizeof(buf));
        buf_ready = true;
        error_report("usb-echo: handle data out, buf: %x", buf[0]);
    } else {
        p->status = USB_RET_STALL;
        error_report("usb-echo: handle data stall");
    }
}

static void usb_echo_handle_reset(USBDevice *dev) {
}

static void usb_echo_handle_realize(USBDevice *dev, Error **errp) {
    usb_desc_init(dev);
}

static const USBDescIface usb_echo_high_iface_desc[] = {
    {
        .bInterfaceNumber              = 0,
        .bNumEndpoints                 = 2,
        .bInterfaceClass               = USB_CLASS_COMM,
        .bInterfaceSubClass            = 0x01,
        .bInterfaceProtocol            = 0x01,
        .eps = (USBDescEndpoint[]) {
            {
                .bEndpointAddress      = USB_DIR_IN | USB_ECHO_EP_IN,
                .bmAttributes          = USB_ENDPOINT_XFER_INT,
                .wMaxPacketSize        = 512,
                .bInterval             = 1,
            },{
                .bEndpointAddress      = USB_DIR_OUT | USB_ECHO_EP_OUT,
                .bmAttributes          = USB_ENDPOINT_XFER_INT,
                .wMaxPacketSize        = 512,
                .bInterval             = 1,
            },
        },
    },
};

static const USBDescDevice usb_echo_high_desc = {
    .bcdUSB                        = 0x0300,
    .bMaxPacketSize0               = 64,
    .bNumConfigurations            = 1,
    .confs = (USBDescConfig[]) {
        {
            .bNumInterfaces        = 1,
            .bConfigurationValue   = 1,
            .iConfiguration        = STR_CONFIG_SUPER,
            .bmAttributes          = USB_CFG_ATT_ONE | USB_CFG_ATT_SELFPOWER,
            .nif = 1,
            .ifs = usb_echo_high_iface_desc,
        },
    },
};

static const USBDesc usb_echo_desc = {
    .id = {
        .idVendor          = USB_ECHO_VENDOR_ID,
        .idProduct         = USB_ECHO_PRODUCT_ID,
        .bcdDevice         = 0x0001,
        .iManufacturer     = STR_MANUFACTURER,
        .iProduct          = STR_PRODUCT,
        .iSerialNumber     = STR_SERIAL,
    },
    .high = &usb_echo_high_desc,
    .str = usb_echo_desc_strings,
};

static void usb_echo_instance_init(Object *obj) {
    USBDevice *dev = USB_DEVICE(obj);
    dev->speed = USB_SPEED_HIGH;
}

static void usb_echo_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);
    USBDeviceClass *uc = USB_DEVICE_CLASS(klass);

    dc->fw_name = "echo";

    uc->product_desc   = "USB Echo";
    uc->usb_desc       = &usb_echo_desc;
    uc->realize        = usb_echo_handle_realize;
    uc->handle_reset   = usb_echo_handle_reset;
    uc->handle_control = usb_echo_handle_control;
    uc->handle_data    = usb_echo_handle_data;
    uc->handle_attach  = usb_desc_attach;
}

static const TypeInfo usb_echo_type_info = {
    .name = "usb-echo",
    .parent = TYPE_USB_DEVICE,
    .class_init = usb_echo_class_init,
    .instance_size = sizeof(USBEchoState),
    .instance_init = usb_echo_instance_init,
};

static void usb_echo_register_types(void) {
    type_register_static(&usb_echo_type_info);
}

type_init(usb_echo_register_types);

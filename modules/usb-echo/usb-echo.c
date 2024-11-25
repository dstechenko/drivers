#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/error-report.h"

#include "hw/usb.h"

#include "desc.h"

typedef struct {
    USBDevice dev;
    uint8_t buf[64];
    size_t buf_len;
} USBEchoState;

enum {
    STR_MANUFACTURER = 1,
    STR_PRODUCT,
    STR_SERIAL,
    STR_CONFIG_HIGH,
};

static const USBDescStrings usb_echo_desc_strings = {
    [STR_MANUFACTURER] = "QEMU",
    [STR_PRODUCT]      = "QEMU USB ECHO",
    [STR_SERIAL]       = "0x42",
    [STR_CONFIG_HIGH] = "High speed config (usb 2.0)",
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

static void usb_echo_handle_data(USBDevice *dev, USBPacket *p) {
    USBEchoState *s = DO_UPCAST(USBEchoState, dev, dev);

    if (p->ep->nr == 1) {
        usb_packet_copy(p, s->buf, sizeof(s->buf));
        // usb_packet_setup(p, USB_TOKEN_IN, 2, s->buf_len);
        // usb_packet_copy(p, s->buf, s->buf_len);
        // usb_packet_complete(p, USB_RET_SUCCESS);
    } else if (p->ep->nr == 2) {
        // usb_packet_setup(p, USB_TOKEN_IN, 2, s->buf_len);
        usb_packet_copy(p, s->buf, s->buf_len);
        // usb_packet_complete(p, USB_RET_SUCCESS);
    } else {
        // usb_packet_complete(p, USB_RET_STALL);
    }
}

static void usb_echo_handle_reset(USBDevice *dev) {
}

static void usb_echo_handle_realize(USBDevice *dev, Error **errp) {
    usb_desc_init(dev);
}

static const USBDescIface usb_echo_high_iface_desc = {
    .bInterfaceNumber              = 0,
    .bNumEndpoints                 = 2,
    .bInterfaceClass               = USB_CLASS_COMM,
    .bInterfaceSubClass            = 0x01,
    .bInterfaceProtocol            = 0x01,
    .eps = (USBDescEndpoint[]) {
        {
            .bEndpointAddress      = USB_DIR_IN | 0x01,
            .bmAttributes          = USB_ENDPOINT_XFER_BULK,
            .wMaxPacketSize        = 512,
        },{
            .bEndpointAddress      = USB_DIR_OUT | 0x02,
            .bmAttributes          = USB_ENDPOINT_XFER_BULK,
            .wMaxPacketSize        = 512,
        },
    }
};

static const USBDescDevice usb_echo_high_desc = {
    .bcdUSB                        = 0x0200,
    .bMaxPacketSize0               = 64,
    .bNumConfigurations            = 1,
    .confs = (USBDescConfig[]) {
        {
            .bNumInterfaces        = 1,
            .bConfigurationValue   = 1,
            .iConfiguration        = STR_CONFIG_HIGH,
            .bmAttributes          = USB_CFG_ATT_ONE | USB_CFG_ATT_SELFPOWER,
            .nif = 1,
            .ifs = &usb_echo_high_iface_desc,
        },
    },
};

static const USBDesc usb_echo_desc = {
    .id = {
        .idVendor          = 0x0042,
        .idProduct         = 0x0042,
        .bcdDevice         = 0x0042,
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
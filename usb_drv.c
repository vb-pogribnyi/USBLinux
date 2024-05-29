#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

const struct usb_device_id usb_drv_id_table[] = {
    {USB_DEVICE(0x0483, 0x1234)},
    {}
};

static struct usb_class_driver usb_drv_class;
static struct usb_device *usb_drv_device;
static __u8 *usb_buffer;

ssize_t usb_drv_read (struct file *file, char __user *buffer, size_t count, loff_t *offset) {
    int res = 0;
    int actual_len = 0;
    
    // __u8 endpoint = 0;
    // __u8 request = 0x00;    // GET_STATUS
    // __u8 requesttype = 0xC0;// TYPE_VENDOR | RECEPIENT_DEVICE
    // __u16 index = 0;
    // __u16 value = 0;
    unsigned int pipe = usb_rcvintpipe(usb_drv_device, 1);

    // usb_control_msg(usb_drv_device, pipe, request, requesttype, value, index,
    //     usb_buffer, 8, 5000);
    usb_interrupt_msg(usb_drv_device, pipe,
	usb_buffer, 8, &actual_len, 5000);
    usb_buffer[0] += 48;
    usb_buffer[1] = '\n';

    res = copy_to_user(buffer, usb_buffer, 8);
    printk("Reading %lu bytes: %s (%i read)\n", count, usb_buffer, actual_len);
    if (*offset < 8) {
        *offset += 8;
        return 8;
    }
    return 0;
}
ssize_t usb_drv_write (struct file *file, const char __user *buffer, size_t count, loff_t *offset) {
    char value_str[8] = {0};
    long value_l = 0;
    int res = 0;
    __u8 endpoint = 0;
    __u8 request = 0x03;    // SET_FEATURE
    __u8 requesttype = 0x40;// TYPE_VENDOR | RECEPIENT_DEVICE
    __u16 index = 0;
	void *data = 0;
    __u16 size = 0;
    unsigned int pipe = usb_sndctrlpipe(usb_drv_device, endpoint);

    res = copy_from_user(value_str, buffer, count);
    res = kstrtol(value_str, 10, &value_l);
    printk("Writing %lu bytes: %s, %li\n", count, value_str, value_l);
    
    usb_control_msg(usb_drv_device, pipe, request, requesttype, (__u16) value_l, index,
        data, size, 5000);

    return count;
}
int usb_drv_open (struct inode *node, struct file *file) {
    return 0;
}


struct file_operations fops = {
    .open=usb_drv_open,
    .read=usb_drv_read,
    .write=usb_drv_write
};

int usb_drv_probe (struct usb_interface *intf, const struct usb_device_id *id) {
    int retval = 0;
    printk("Probing device\n");
    usb_drv_class.name = "usb/usbdrv_%d";
    usb_drv_class.fops = &fops;
    usb_drv_device = interface_to_usbdev(intf);
    if ((retval = usb_register_dev(intf, &usb_drv_class)) < 0) {
        printk("Could not register device\n");
    } else {
        printk("Minor obtained: %d\n", intf->minor);
    }
    return retval;
}

void usb_drv_disconnect(struct usb_interface *intf) {
    printk("Disconnecting device\n");
    usb_deregister_dev(intf, &usb_drv_class);
}

struct usb_driver usb_drv = {
    .name="Test USB driver",
    .probe=usb_drv_probe,
    .disconnect=usb_drv_disconnect,
    .id_table=usb_drv_id_table
};

int __init usb_drv_init(void) {
    printk("Initializing\n");
    usb_buffer = kmalloc(8, GFP_KERNEL);
    usb_register(&usb_drv);
    return 0;
}

void __exit usb_drv_exit(void) {
    printk("Exiting\n");
    usb_deregister(&usb_drv);
    kfree(usb_buffer);
}

module_init(usb_drv_init);
module_exit(usb_drv_exit);

MODULE_LICENSE("GPL");

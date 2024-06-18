#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <linux/delay.h>
#define NETLINK_USER 31

const struct usb_device_id usb_drv_id_table[] = {
    {USB_DEVICE(0x0483, 0x1234)},
    {}
};

static struct usb_class_driver usb_drv_class;
static struct usb_device *usb_drv_device;
static __u8 *usb_buffer;
// static bool is_probing = false;
static struct urb *urb;
static __u8 *urb_buffer;
static struct sock *nl_sock;
static int pid;

uint8_t* tmp_buff;

static void nl_receive(struct sk_buff *skb) {
    struct nlmsghdr *nlh;
    nlh = (struct nlmsghdr*)skb->data;
    pid = nlh->nlmsg_pid;
    printk("Received message from pid %i: %s\n", pid, (char*)nlmsg_data(nlh));
}

int try_send_audio(uint8_t* buff, uint16_t count) {
    __u8 endpoint = 0;
    __u8 request = 0x03;    // SET_FEATURE
    __u8 requesttype = 0xC1;// TYPE_VENDOR | RECEPIENT_DEVICE
    __u16 index = 0;
    __u16 value = count;
    int res = 0;
    unsigned int pipe;
    unsigned int pipe_blk;
    int actual_length;
    pipe = usb_rcvctrlpipe(usb_drv_device, endpoint);
    pipe_blk = usb_sndbulkpipe(usb_drv_device, 2);

    usb_control_msg(usb_drv_device, pipe, request, requesttype, value, index,
        usb_buffer, 8, 5000);
    if (usb_buffer[0] == 1) {
        // Send audio using bulk transaction
        printk("Sengind audio: %i bytes\n", count);
        res = usb_bulk_msg(usb_drv_device, pipe_blk, buff, count, &actual_length, 1000);
        if (res) printk("Error sending request: %i\n", res);
        printk("Sent: %i\n", actual_length);
        return actual_length;
    } else {
        // There's no space available on the device. Wait.
        return 0;
    }
}

ssize_t usb_drv_read (struct file *file, char __user *buffer, size_t count, loff_t *offset) {
    int res = 0;
    int actual_len = 0;
    
    __u8 endpoint = 0;
    __u8 request = 0x00;    // GET_STATUS
    __u8 requesttype = 0xC0;// TYPE_VENDOR | RECEPIENT_DEVICE
    __u16 index = 0;
    __u16 value = 0;
    unsigned int pipe = usb_rcvctrlpipe(usb_drv_device, endpoint);

    usb_control_msg(usb_drv_device, pipe, request, requesttype, value, index,
        usb_buffer, 8, 5000);
    // usb_interrupt_msg(usb_drv_device, pipe, usb_buffer, 8, &actual_len, 5000);
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
    int res = 0;
    size_t bytes_sent = 0;
    int packet_size = 2048;
    int retries = 0;

    printk("Requesting audio transmittion\n");
    tmp_buff = kmalloc(count, GFP_KERNEL);
    res = copy_from_user(tmp_buff, buffer, count);

    // Do send in a loop
    while (bytes_sent < count && retries++ < 1000) {
        if (packet_size > count) packet_size = count;
        bytes_sent += try_send_audio(tmp_buff + bytes_sent, packet_size);
        printk("Total sent: %li bytes\n", bytes_sent);
        msleep(5);
    }

    kfree(tmp_buff);
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

static void usb_int_callback(struct urb* urb) {
    struct sk_buff *skb_out = nlmsg_new(1, 0);
    struct nlmsghdr *nlh;
    int res;

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, 1, 0);
    NETLINK_CB(skb_out).dst_group = 0;
    strncpy(nlmsg_data(nlh), urb_buffer, 1);
    res = nlmsg_unicast(nl_sock, skb_out, pid);
    if (res < 0) {
        printk("NLMSG error: %i\n", res);
    }
    printk("Interrupt happened: %i\n", urb_buffer[0]);
    if (usb_submit_urb(urb, GFP_KERNEL)) {
        printk("Failed to RE-submit URB\n");
    }

}

int usb_drv_probe (struct usb_interface *intf, const struct usb_device_id *id) {
    int retval = 0;
    struct usb_host_endpoint *ep;
    unsigned int pipe;

    printk("Probing device\n");
    usb_drv_class.name = "usb/usbdrv_%d";
    usb_drv_class.fops = &fops;
    usb_drv_device = interface_to_usbdev(intf);
    if ((retval = usb_register_dev(intf, &usb_drv_class)) < 0) {
        printk("Could not register device\n");
    } else {
        printk("Minor obtained: %d\n", intf->minor);
    }

    // Start interrupt requests
    urb = usb_alloc_urb(0, GFP_KERNEL);
    pipe = usb_rcvintpipe(usb_drv_device, 1);
    urb_buffer = kmalloc(8, GFP_KERNEL);

	ep = usb_pipe_endpoint(usb_drv_device, pipe);
    usb_fill_int_urb(urb, usb_drv_device, pipe, urb_buffer, 8, 
        usb_int_callback, NULL, ep->desc.bInterval);
    if (usb_submit_urb(urb, GFP_KERNEL)) {
        kfree(urb_buffer);
        usb_free_urb(urb);
        printk("Failed to submit URB\n");
    }


    return retval;
}

void usb_drv_disconnect(struct usb_interface *intf) {
    printk("Disconnecting device\n");
    kfree(urb_buffer);
    usb_free_urb(urb);
    usb_deregister_dev(intf, &usb_drv_class);
}

struct usb_driver usb_drv = {
    .name="Test USB driver",
    .probe=usb_drv_probe,
    .disconnect=usb_drv_disconnect,
    .id_table=usb_drv_id_table
};

int __init usb_drv_init(void) {
    struct netlink_kernel_cfg cfg = {
        .input=nl_receive,
    };
    printk("Initializing\n");
    usb_buffer = kmalloc(8, GFP_KERNEL);
    usb_register(&usb_drv);
    
    nl_sock = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!nl_sock) {
        printk("Unable to create socket\n");
    }
    return 0;
}

void __exit usb_drv_exit(void) {
    printk("Exiting\n");
    netlink_kernel_release(nl_sock);
    usb_deregister(&usb_drv);
    kfree(usb_buffer);
}

module_init(usb_drv_init);
module_exit(usb_drv_exit);

MODULE_LICENSE("GPL");

import usb.core
import usb.util

dev = usb.core.find(idVendor=0x0483, idProduct=0x1234)
print(dev)
dev.write(2, 's11122200000000\n')


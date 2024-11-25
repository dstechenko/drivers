obj-m += usb-ipc.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
install:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules_install
	/sbin/rmmod  usb-ipc.ko || true
	/sbin/insmod usb-ipc.ko
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
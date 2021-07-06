obj-m := pageFaultHandler.o

KDIR = /lib/modules/$(shell uname -r)/build

default:
	make -C $(KDIR) M=$(shell pwd) modules
clean:
	make -C $(KDIR) M=$(shell pwd) clean



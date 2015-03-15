ifneq ($(KERNELRELEASE),)
	scull-objs := device.o main.o fops.o
	obj-m := scull.o
else 
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

clean:
	rm *.o *.ko *.order *.symvers  *.mod.c

ifneq ($(KERNELRELEASE),)

	obj-m := paging.o
	CFLAGS_paging.o := -DDEBUG -g

else

	KERNELDIR ?= /lib/modules/$(shell uname -r)/build

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif

ifneq ($(KERNELRELEASE),)
	obj-m := tufilter_core.o
else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

module_core: tufilter
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
tufilter: tufilter.c
	gcc tufilter.c -std=c99 -o tufilter

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm tufilter

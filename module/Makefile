ifneq ($(KERNELRELEASE),)

obj-m := modzpu.o

else

KDIR := /lib/modules/$(shell uname -r)/build
PWd := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

install:
	scripts/load.sh modzpu

uninstall:
	scripts/unload.sh modzpu
endif

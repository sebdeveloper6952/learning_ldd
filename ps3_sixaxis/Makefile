obj-m := ps3_sixaxis.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

all:
	make -C $(KERNELDIR) M=$(PWD)

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c *.mod .tmp_versions
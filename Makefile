ifneq ($(KERNELRELEASE),)

obj-m := lastwords.o
lastwords-objs := lastwords_main.o lastwords_mem.o lastwords_record.o lastwords_interface.o lastwords_export.o lastwords_monitor.o lastwords_sym.o

else
	
KDIR := /home/apple/raspberry/build/linux-rpi-4.1.y
all:prepare
	make -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=arm-bcm2708-linux-gnueabi-
	cp *.ko ../release/

	arm-bcm2708-linux-gnueabi-gcc -o lastwordstools lastwords_user.c
	cp lastwordstools ../release/
prepare:
	cp /home/apple/win_share/lastwords/* ./
	mkdir -p ../release
modules_install:
	make -C $(KDIR) M=$(PWD) modules_install ARCH=arm CROSS_COMPILE=arm-bcm2708-linux-gnueabi-
clean:
	rm -f *.ko *.o *.mod.o *.mod.c .*.cmd *.symvers  modul*

endif

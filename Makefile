obj-m += SWDPI.o
my_module-objs+= SWDPI_raspi4.o SWDPI_raspi5.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

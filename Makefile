
SWDPI-objs:= SWDPI_base.o SWDPI_raspi4.o SWDPI_raspi5.o SWDPI_SWD.o
obj-m += SWDPI.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

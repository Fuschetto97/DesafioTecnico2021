obj-m 		+= i2c_dev.o
PWD 		:= $(shell pwd)
KERNEL_DIR 	= /home/martin/Escritorio/Imagen_BBB/bb-kernel/deploy/4.18.20-bone16-modules/lib/modules/4.18.20-bone16/build

cross:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KERNEL_DIR) M=$(PWD) LDDINC=$(PWD)/../include modules
cleancross:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KERNEL_DIR) M=$(PWD) clean
sendcross:
	scp i2c_dev.ko ubuntu@192.168.6.2:/home/ubuntu

compile: ./src/i2c_dev.c
	@echo Generando $@.o...				
	mkdir -p bin
	$(CC) $(CFLAG) $(BIN_DIR)$@.bin $(SRC_DIR)$@.c -Wall
cleancompile:
	@echo Limpiando $@.o...	
	rm -f -r ./sup/*.lst ./bin/* ./bin/*.bin ./obj/*.o

main: ./main.c
	@echo Generando $@.o...				
	arm-linux-gnueabihf-gcc -o $@.bin $@.c -Wall
clean:
	rm -f -r ./sup/*.lst ./bin/* ./bin/*.bin ./obj/*.o
sendmain:
	scp main.bin ubuntu@192.168.6.2:/home/ubuntu


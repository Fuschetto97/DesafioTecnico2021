CC			= gcc
CFLAG 			= -o

CloudCommunicationApp: ./src/CloudCommunicationApp.c
	@echo Generando $@.o...				
	$(CC) $(CFLAG) ./bin/$@.bin ./src/$@.c ./src/initsem.c -Wall

CPUtemp: ./src/CPUtemp.c
	@echo Generando $@.o...				
	$(CC) $(CFLAG) ./bin/$@.bin ./src/$@.c ./src/initsem.c -Wall

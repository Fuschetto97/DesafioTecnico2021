CFLAG 				= -o 
#CC					= gcc
CC 					= arm-linux-gnueabihf-gcc

Servidor: 
	@echo Generando $@.o...				
	$(CC) -o Servidor.bin Servidor.c ProcesarCliente.c Config.c Server.c initsem.c Server_Reader_Child.c  ProcesadorEstadistico.c -Wall 

Client: ./src/Client.c
	@echo Generando $@.o...				
	$(CC) $(CFLAG) $@.bin $@.c -Wall

clean:
	rm -f -r ./sup/*.lst ./bin/* ./*.bin ./obj/*.o

sendmain:
	scp /home/martin/Escritorio/RepoTD3/r5054-td3-Fuschetto-Martin/02_cuat/TPO_ClienteServidor/Servidor.bin ubuntu@192.168.6.2:/home/ubuntu/Server

dir: 
	touch README

andmore:
	make clean
	make
	make bochs
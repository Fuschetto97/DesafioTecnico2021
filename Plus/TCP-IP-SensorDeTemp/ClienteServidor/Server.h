#if !defined( HEADERSV )
#define HEADERSV


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define PORT            9876    // puerto

#define DEFAULT_BACKLOG 2       // max cant de conexiones encoladas
#define DEFAULT_PROCESS 1000    // 1000 conexiones activas
#define DEFAULT_ACQTIME 1       // 1 seg
#define DEFAULT_AVERAGE 5       // 5 promedios de las muestras del driver

void ProcesarCliente(int new_sfd, struct sockaddr_in *their_addr, int semid, char *buffer);
void Config(int *backlog, int *process, int *acqtime, int *average);
void Server(int *backlog, int *process, int *acqtime, int *average, char *buffer, int semid, int shmid, int fdp);
void Server_Reader_Child(int *backlog, int *process, int *acqtime, int *average, int *size, int buffer_size, char *buffer, int semid, int shmid, struct sembuf *sb, int fdp);
float procesador_estadistico(float *data, int average);
int initsem(key_t key, int nsems);

union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};

#endif 
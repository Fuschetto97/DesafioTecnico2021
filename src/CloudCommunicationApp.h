#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include <sys/sem.h>

#define SHM_SIZE 1024   // 1K 
#define SOCKET_PATH "echo_socket"
#define SIZE_SENDED 100

#define MAX_RETRIES 10

int initsem(key_t, int);

union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};

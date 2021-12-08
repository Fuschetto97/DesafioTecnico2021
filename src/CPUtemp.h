#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <unistd.h>

#define SHM_SIZE 200   // 1K 
#define DIR_CPU_TEMP_SENSOR "/sys/class/hwmon/hwmon0/device/temp1_input"

int initsem(key_t, int);

union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};
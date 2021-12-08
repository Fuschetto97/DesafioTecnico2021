/* 
Funcion : Este proceso actuara como productor, se encargara de conseguir la temperatura del CPU de la PC y luego escribirla en una memoria compartida.

Creador : Fuschetto Martin
*/ 

#include "./CPUtemp.h"

volatile int f_close=0;     // Evita optimizaciones con f_close

void sigusr1_handler(int s)
{
    f_close=1;
}

int main(void)
{
    // Variables para shr mem
    int shmid=0;           
    char *buffer=NULL;
    key_t shrmem_key;

    // Variables para el semaforo
    key_t sem_key;              // para semaforo
    int semid, fdp;
    struct sembuf sb;

    // Variables lectura de temperatura
    double temp;       // muestra de temperatura
    int res;

    // Variable fd 
    FILE *fd_file;

    // File open
    if ((fd_file=fopen(DIR_CPU_TEMP_SENSOR, "r")) == NULL)
    {
        perror("open");
        exit(1);
    }

    // Inicializacion de share memory
    // Creo el key
    if ((shrmem_key = ftok("CloudCommunicationApp.c", 'R')) == -1) {    // Uso el mismo archivo
        perror("ftok");
        exit(1);
    }
    // Conecto al segmento (id del segmento de memoria)
    if ((shmid = shmget(shrmem_key, SHM_SIZE, 0644 | IPC_CREAT)) == -1) {
    perror("shmget");
    exit(1);
    }

    // Adjunto el segmento a un puntero
    buffer=(char*)shmat(shmid,NULL,0);
    if(buffer<0){
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        exit(1);
    }

    // Inicializacion del semaforo 
    if((fdp=open("SemFile", O_RDONLY | O_CREAT, 0777))==-1){
        perror(" Open");
        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        exit(1);
    }
    if((sem_key = ftok("SemFile", 'E'))==-1){
        perror(" ftok ");
        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        close(fdp);
        exit(1);
    }
    if ((semid = initsem(sem_key, 1)) == -1) {      /* Configura el semaforo creado semid */
        perror("initsem");
        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        close(fdp);
        exit(1);
    }

    // Leo la temperatura del CPU
    res = fscanf( fd_file, "%lf", &temp);
    if (res == EOF) {
        fprintf(stderr, "Error: fscanf alcanzo el final del archivo y no hubo matching, errno: %s\n", strerror(errno) );
        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        close(fdp);
        return -1;
    }
    else if (res != 1) {    // Chequeo que haya matchado y asignado la cantidad entradas correspondientes
        fprintf(stderr, "Error: fscanf matcheo y asigno %i entradas, se esperaba 1 \n", res);
        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        close(fdp);
        return -1;
    }
    
    while( f_close == 1)
    {
        // Espero a que el semaforo sea liberado si es que fue tomado
        sb.sem_op = -1;         // Espero a que sea liberado y lo tomo
        if (semop(semid, &sb, 1) == -1) {   // semop asigna, libera o espera a que sea liberado 
            perror("semop");
            shmdt(buffer);
            shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
            close(fdp);
            exit(1);
        }

        // Procedo a escribir la shr mem
        snprintf(buffer, SHM_SIZE, "La temperatura es = %f", (temp));

        sb.sem_op = 1;          /* Libera el recurso, lo dejo disponible para otro proceso */
        if (semop(semid, &sb, 1) == -1) {
            perror("semop");
            shmdt(buffer);
            shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
            close(fdp);
            exit(1);
        }

        sleep(1);
    }

    // Libero recursos
    shmdt(buffer);
    shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
    close(fdp);
    return 0;
}

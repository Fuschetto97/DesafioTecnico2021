#include "Server.h"

volatile int f_update=0;
volatile int f_close=0;

void sigchld_handler(int signum)
{
    int status;
    printf("Recibi la señal SIGCHLD; signum : %d\n", signum);
    while(waitpid(-1, &status, WNOHANG) > 0)      /* Wait for any child process, no hand */
    {    
        printf("status tiene : %d\n", WIFEXITED(status));
        if(WIFEXITED(status) != 1)
        {
            printf(" Un hijo no pudo terminar bien\n");
            exit(1);        // Preguntar
        }
    }
}

void sigusr1_handler(int s)
{
    f_update=1;
}

void sigint_handler(int s)
{
    f_close=1;
}

int main(void)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////// Inicializacion ////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int pid;
    struct sigaction sa_chld, sa_usr1;

    int backlog=DEFAULT_BACKLOG;
    int process=DEFAULT_PROCESS;
    int acqtime=DEFAULT_ACQTIME;
    int average=DEFAULT_AVERAGE;

    int size=0;             // para shm mem
    int shmid=0;           
    char *buffer=NULL;
    size_t buffer_size = 200;

    key_t key;              // para semaforo
    int semid, fdp;
    struct sembuf sb;

    sb.sem_num = 0;         /* Numeros de semaforos a manipular */
    sb.sem_op = -1;         /* Asignar recursos */
    sb.sem_flg = SEM_UNDO;  /* Restaura la config del semaforo */

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////// Lectura de fichero/config por defecto //////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    printf(" SERVER: Lectura de archivo config... \n");
    Config(&backlog, &process, &acqtime, &average);
    printf(" SERVER: Lectura de archivo config terminada\n");
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////// Inicializar shm mem  //////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    shmid=shmget(IPC_PRIVATE, buffer_size, IPC_CREAT | 0666);   // read and write permissions for all users
    if (shmid < 0){
        perror("shmget");
        exit(1);
    }
    buffer=(char*)shmat(shmid,NULL,0);
    if(buffer<0){
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        exit(1);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////// Inicializar semaforo  /////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if((fdp=open("SemFile", O_RDONLY | O_CREAT, 0777))==-1){
        perror(" Open");
        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        exit(1);
    }
    if((key = ftok("SemFile", 'E'))==-1){
        perror(" ftok ");
        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        close(fdp);
        exit(1);
    }
    if ((semid = initsem(key, 1)) == -1) {      /* Configura el semaforo creado semid */
        perror("initsem");
        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        close(fdp);
        exit(1);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////// Inicializacion de Señales ///////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    sa_chld.sa_handler = sigchld_handler;   // reap all dead processes
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        perror("sigaction");
        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        close(fdp);
        semctl(semid, 0, IPC_RMID);  //Elimino semaforo
        exit(1);
    }

    sa_usr1.sa_handler = sigusr1_handler;   // config SIGUSR1
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        perror("sigaction");
        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        close(fdp);
        semctl(semid, 0, IPC_RMID);  //Elimino semaforo
        exit(1);
    }

    sa_chld.sa_handler = sigint_handler;   // config SIGINT
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa_chld, NULL) == -1) {
        perror("sigaction");
        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        close(fdp);
        semctl(semid, 0, IPC_RMID);  //Elimino semaforo
        exit(1);
    }



    printf(" SERVER: Creo un hijo lector del driver...\n");
    switch (pid=fork())
    {
        case -1:
            perror("fork_1"); /* something went wrong */
            printf("Oh dear, something went wrong with fork()! %s\n", strerror(errno));
            shmdt(buffer);
            shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
            close(fdp);
            semctl(semid, 0, IPC_RMID);  //Elimino semaforo
            exit(1); /* parent exits */
            break;
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////// Servidor lector hijo ////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        case 0:         /* Proceso hijo */
            printf(" CHILD READER DRIVER: HI! My pid is %d\n", getpid());
            Server_Reader_Child(&backlog, &process, &acqtime, &average, &size, buffer_size, buffer, semid, shmid, &sb, fdp);
            printf(" CHILD READER DRIVER: Me voy\n");
            shmdt(buffer);      // Cuando el hijo es creado hereda todo lo de su padre, lo cierro antes de cerrar al hijo
            shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
            close(fdp);
            semctl(semid, 0, IPC_RMID);  //Elimino semaforo
            exit(0);
            break;
        default:        /* Proceso padre*/
            printf(" SERVER: HI! My pid is %d\n", getpid());
            Server(&backlog, &process, &acqtime, &average, buffer, semid, shmid, fdp);
            printf(" SERVER: Me voy\n");
            break;
    }
    return(0);
}


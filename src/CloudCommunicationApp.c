/* 
Funcion : Este proceso actuara como consumidor, al mismo tiempo se debe de encargar de la comunicacion con el servidor Cloud.
Creador : Fuschetto Martin
Metodo de comunicacion: Share Memory: Segmento de memoria que se comparte entre procesos.

*/ 
#include "./CloudCommunicationApp.h"

volatile int f_close=0;     // Evita optimizaciones con f_close

void sigusr1_handler(int s)
{
    f_close=1;
}

int main(void)
{
    // Variables para el cliente
    int s, t, len;
    struct sockaddr_un remota;
    char str[SIZE_SENDED];

    // Variables para shr mem
    int shmid=0;           
    char *buffer=NULL;
    key_t shrmem_key;

    // Variables para el semaforo
    key_t sem_key;              // para semaforo
    int semid, fdp;
    struct sembuf sb;

    // Variables para la Lectura
    char Trx[200];
    float tempCelsius;

    // Variables para signals
    struct sigaction sa_usr1;

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

    // Inicializacion de signals
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

    // 1. Llamar a socket () para obtener un socket (Unix) para comunicarse.   
    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        close(fdp);
        semctl(semid, 0, IPC_RMID);  //Elimino semaforo
        exit(1);
    }

    // 2. Configurar una estructura sockaddr_un ("remota") con la dirección remota (donde el servidor está escuchando) y 
    // llamar a connect () con ese argumento. 
    
    // En este punto el sistema operativo te da un puerto (dato de color: superior al 1024 ya que los primeros los 
    // tiene reservados, no es prestador de servicio unicamente sirve para escuchar). 

    printf("Intentando conectar...\n");
    remota.sun_family = AF_UNIX;
    strcpy(remota.sun_path, SOCKET_PATH);
    len = strlen(remota.sun_path) + sizeof(remota.sun_family);

    if (connect(s, (struct sockaddr *)&remota, len) == -1) {
        perror("connect");
        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        close(fdp);
        semctl(semid, 0, IPC_RMID);  //Elimino semaforo
        close(s);
        exit(1);
    }
    printf("Conectado.\n");

    while( f_close == 1)
    {
        // Espero a que el semaforo sea liberado si es que fue tomado
        sb.sem_op = -1;         // Espero a que sea liberado y lo tomo
        if (semop(semid, &sb, 1) == -1) {   // semop asigna, libera o espera a que sea liberado 
            perror("semop");
            shmdt(buffer);
            shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
            close(fdp);
            semctl(semid, 0, IPC_RMID);  //Elimino semaforo
            close(s);
            exit(1);
        }

        // Procedo a leer la shr mem
        switch (sscanf(buffer, "La temperatura es = %s", Trx))
        {
            case EOF:       // Error
                printf("Aun no hay nada escrito en la shr mem\n");
                tempCelsius=0;
                break;
            default:        // Encontro
                tempCelsius=atof(Trx);
                printf("temp es %f\n", tempCelsius);
                break;
        }

        sb.sem_op = 1;          /* Libera el recurso, lo dejo disponible para otro proceso */
        if (semop(semid, &sb, 1) == -1) {
            perror("semop");
            shmdt(buffer);
            shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
            close(fdp);
            semctl(semid, 0, IPC_RMID);  //Elimino semaforo
            close(s);
            exit(1);
        }

        // Aqui se le puede dar cierto tratamiento a la muestra de temperatura recibida
        // desde otro proceso.

        snprintf(str, SIZE_SENDED, "La temperatura es = %f", tempCelsius);

        //3. Suponiendo que no haya errores, nos conectamos al lado remoto. Se utiliza send() y recv() para la comunicacion.
        if (send(s, str, strlen(str), 0) == -1) {
            perror("send");
            shmdt(buffer);
            shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
            close(fdp);
            semctl(semid, 0, IPC_RMID);  //Elimino semaforo
            close(s);
            exit(1);
        }

        // Aqui espero recibir del "Cloud Server" algo asi como un "okey, gracias"
        if ((t=recv(s, str, 100, 0)) > 0) 
        {
            str[t] = '\0';
            printf("echo> %s", str);
        } else {
            if (t < 0) 
                perror("recv");
            else // t=0
                printf("Server closed connection\n");
                exit(1);
        }

        sleep(1);
    }

    // Libero recursos
    shmdt(buffer);
    shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
    close(fdp);
    semctl(semid, 0, IPC_RMID);  //Elimino semaforo
    close(s);
    return 0;
}

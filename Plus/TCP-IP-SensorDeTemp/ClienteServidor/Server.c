#include "Server.h"

extern int f_update;
extern int f_close;

void Server(int *backlog, int *process, int *acqtime, int *average, char *buffer, int semid, int shmid, int fdp)
{
    int sfd;
    int yes=1;
    socklen_t sin_size;
    struct sockaddr_in mi_addr;         // my address information
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////// Inicializacion de Servidor //////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    printf(" SERVER: Inicializacion del servidor... \n");
    /* Crea un endpoint para la comunicacion, domain: familia de protocolo AF_INET ipv4 protoculo de la capa de red */
    /* SOCK STREAM: Para comunicaciones confiables en modo conectado, de dos vıas y con tamano variable de los mensajes de datos */

    if((sfd=socket(AF_INET, SOCK_STREAM, 0))==-1){          /* Entrega file descriptor for the new socket */
        perror("socket");
        shmdt(buffer);      // Cuando el hijo es creado hereda todo lo de su padre, lo cierro antes de cerrar al hijo
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        close(fdp);
        semctl(semid, 0, IPC_RMID);  //Elimino semaforo
        exit(1);
    }

    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
        perror("setsockopt");
        shmdt(buffer);      // Cuando el hijo es creado hereda todo lo de su padre, lo cierro antes de cerrar al hijo
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        close(fdp);
        semctl(semid, 0, IPC_RMID);  //Elimino semaforo
        close(sfd);
        exit(1);
    }
    /* Tenemos el socket pero asi solito no sirve tenemos que asignarle un port, una ip y servicio con bind */
    /* el primer int es el fd del socket, el segundo es una estructura sockaddr en donde se escribe ip:puerto. si usas 
    sockaddr_in hay que castear */
    /* por ultimo addrlen tamanio de la estructura */
    bzero((char *) &mi_addr, sizeof(mi_addr)); /* Lleno de ceros la estructura */
    mi_addr.sin_family = AF_INET;       /* ipv4 internet protocol */
    mi_addr.sin_port = htons(PORT);     /* Como intel es little endian y la red usa big endian transformo */
    mi_addr.sin_addr.s_addr = INADDR_ANY;  /* INADDR_ANY: Devuelve la ip local activa en big endian */

    if((bind(sfd, (struct sockaddr *)&mi_addr, sizeof(mi_addr)))==-1){
        perror("bind");
        shmdt(buffer);      // Cuando el hijo es creado hereda todo lo de su padre, lo cierro antes de cerrar al hijo
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        close(fdp);
        semctl(semid, 0, IPC_RMID);  //Elimino semaforo
        close(sfd);
        exit(1);
    }
    /* Listen, s : fd del socket. backlog : cantidad de pedidos de conexion que el proceso almacenara mientras responde al pedido de conexion. Son bits de test 
    que se mandan entre nodos hasta establecer la conexion */
    if(listen(sfd, DEFAULT_BACKLOG)==-1){
        perror("listen");
        shmdt(buffer);      // Cuando el hijo es creado hereda todo lo de su padre, lo cierro antes de cerrar al hijo
        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
        close(fdp);
        semctl(semid, 0, IPC_RMID);  //Elimino semaforo
        close(sfd);
        exit(1);
    }

    while(1)
    {
        pid_t pid;                          // para fork
        int new_sfd;                        // nuevos sockets
        struct sockaddr_in their_addr;      // their address information

        fd_set rfds;                        // para select
        int selret, n;                      


        FD_ZERO(&rfds);
        FD_SET(sfd, &rfds);     // listener

        n=sfd;          // el unico fd


        //printf(" SERVER: Antes de select\n");
        selret=select(n+1, &rfds, NULL, NULL, NULL);
        //printf(" SERVER: Despues select\n");

        if (selret == -1){                  
            if (errno != EINTR){
                printf("error de select");
                shmdt(buffer);      // Cuando el hijo es creado hereda todo lo de su padre, lo cierro antes de cerrar al hijo
                shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
                close(fdp);
                semctl(semid, 0, IPC_RMID);  //Elimino semaforo
                close(sfd);
                exit(1);
            }
            else
            {
                printf(" SERVER: A signal was caught\n");
                ///////////////////////////////////////////////////////////////////////////////////////////////////////////
                /////////////////////////////// Actualizacion de cond. de operacion ///////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////////////////////
                if(f_update)
                {
                    printf(" SERVER: La signal fue SIGUSR1 \n");
                    printf(" SERVER: Lectura de archivo config... \n");
                    Config(backlog, process, acqtime, average);
                    f_update=0;
                    printf(" SERVER: Lectura de archivo config terminada\n");
                }
                if(f_close)
                {
                    printf(" SERVER: La signal fue SIGINT \n");
                    shmdt(buffer);      // Cuando el hijo es creado hereda todo lo de su padre, lo cierro antes de cerrar al hijo
                    shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
                    close(fdp);
                    semctl(semid, 0, IPC_RMID);  //Elimino semaforo
                    close(sfd);
                    f_update=0;
                    exit(0);
                }
            }
        }
        else{
            if(FD_ISSET(sfd, &rfds)){
                printf(" SERVER: Nuevo pedido\n");
                ///////////////////////////////////////////////////////////////////////////////////////////////////////////
                /////////////////////////////// Espera de conexiones //////////////////////////////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////////////////////////
                /* Accept: Extrae la primera coneccion requerida en la cola de conexiones pendientes para el socket que esta escuchando 
                therir_addr es donde devuelve la informacion sobre la conexion entrante, que host y desde que port te llaman */
                sin_size=sizeof(their_addr);
                if((new_sfd=accept(sfd, (struct sockaddr *)&their_addr, &sin_size))==-1){
                    perror("accept");
                    shmdt(buffer);      // Cuando el hijo es creado hereda todo lo de su padre, lo cierro antes de cerrar al hijo
                    shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
                    close(fdp);
                    semctl(semid, 0, IPC_RMID);  //Elimino semaforo
                    close(sfd);
                    exit(1);
                }
                printf(" SERVER: Tengo una conexion con  %s\n", inet_ntoa(their_addr.sin_addr));
                switch (pid=fork()){
                    case -1:
                        perror("fork"); /* something went wrong */
                        printf("Oh dear, something went wrong with fork()! %s\n", strerror(errno));
                        shmdt(buffer);      // Cuando el hijo es creado hereda todo lo de su padre, lo cierro antes de cerrar al hijo
                        shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
                        close(fdp);
                        semctl(semid, 0, IPC_RMID);  //Elimino semaforo
                        close(sfd);
                        close(new_sfd);
                        exit(1); /* parent exits */
                        break;
                    case 0:         /* Proceso hijo */
                        printf(" CHILD: HI! My pid is %d\n", getpid());
                        close(sfd);         /* El hijo no necesita al socket que escucha */
                        ProcesarCliente(new_sfd, &their_addr, semid, buffer);
                        printf(" CHILD: Me voy\n");
                        // DUDA
                        //shmdt(buffer);      // Cuando el hijo es creado hereda todo lo de su padre, lo cierro antes de cerrar al hijo
                        //shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
                        //close(fdp);
                        //semctl(semid, 0, IPC_RMID);  //Elimino semaforo
                        //close(new_sfd);
                        exit(0);
                        break;
                    default:        /* Proceso padre*/
                        printf(" SERVER: HI! My pid is %d\n", getpid());
                        close(new_sfd);     /* El padre no necesita el socket que se usara como intercambio */
                        //printf(" SERVER: Me voy\n");
                        break;
                }
            }
        }
    }
}
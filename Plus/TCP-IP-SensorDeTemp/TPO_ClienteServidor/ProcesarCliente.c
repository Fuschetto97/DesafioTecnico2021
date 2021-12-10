#include "Server.h"

void ProcesarCliente(int new_sfd, struct sockaddr_in *their_addr, int semid, char *buffer)
{
    char bufferComunic[4096];
    char ipAddr[20];
    int Port;
    //int indiceEntrada;
    float tempCelsius, presHpa;
    char HTML[4096];
    char encabezadoHTML[4096];

    int semval;
    char Trx[200], Prx[200];
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////// Reestaurar Signals //////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (signal(SIGCHLD, SIG_DFL) == SIG_ERR){
        perror("signal");
        exit(1);
    }
    if (signal(SIGUSR1, SIG_DFL) == SIG_ERR){
        perror("signal");
        exit(1);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////// ProcesarCliente ////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    strcpy(ipAddr, inet_ntoa(their_addr->sin_addr));
    Port = ntohs(their_addr->sin_port);
    /* Recibe mensaje del cliente */
    if(recv(new_sfd, bufferComunic, sizeof(bufferComunic), 0) == -1)
    {
        perror("Error en recv");
        exit(1);
    }
    printf(" CHILD: Recivido del navegador web %s:%d: %s\n", ipAddr, Port, bufferComunic);

    semval=semctl(semid, 0, GETVAL);
    //printf("El valor del semaforo es: %d \n", semval);
    if(semval){     // si el semaforo no fue tomado... 
        switch (sscanf(buffer, "La temperatura es = %s La presion es = %s", Trx, Prx))
        {
            case EOF:       // Error
                printf("Aun no hay nada escrito en la shr mem\n");
                tempCelsius=0;
                presHpa=0;
                break;
            default:        // Encontro
                tempCelsius=atof(Trx);
                presHpa=atof(Prx);
                printf("temp es %f\n", tempCelsius);
                printf("pres es %f\n", presHpa);
                break;
        }
    }

    /* Generar HTML */
    sprintf(encabezadoHTML, "<html><head><title>Temperatura y Presion</title>"
                "<meta name=\"viewport\" "
                "content=\"width=device-width, initial-scale=1.0\">"
                "</head>"
                "<h1>Temperatura y Presion</h1>");

    sprintf(HTML, "%s<p> La temperatura de mi cuarto %.2f C y la presion %.2f hPa</p", encabezadoHTML, tempCelsius, presHpa);

    sprintf(bufferComunic, 
            "HTTP/1.1 200 OK\n"
            "Content-Lenght: %d\n"
            "Content-Type: text/html; charset=utg-8\n"
            "Connection: Closed\n\n%s", 
            strlen(HTML), HTML);
    
    printf(" CHILD: Enviado al navegador Web %s:%d:\n%s\n", ipAddr, PORT, bufferComunic);

    /* Enviar mnj al cliente */
    if(send(new_sfd, bufferComunic, strlen(bufferComunic), 0) == -1)
    {
        perror("Error en send");
        exit(1);
    }
    close(new_sfd);
    return;
}

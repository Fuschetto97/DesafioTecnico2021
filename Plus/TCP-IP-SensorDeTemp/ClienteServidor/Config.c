#include "Server.h"

#define LINE_LENGTH 20

void Config(int *backlog, int *process, int *acqtime, int *average)
{
    char sparam[200];

    FILE* ffd;
    int fd;
    int custom=0;      // 0: default, 1: uso archivo de configuracion
    int change=0;
    
    ssize_t read; 
    char * line = NULL;
    size_t len = 0;  

    if ((fd=open("config", O_RDONLY)) == -1)
    {
        switch (errno)
        {
            case ENOENT:        /* No existe el archivo o el directorio, uso valores default */
                printf(" No encontre archivo de configuracion, voy a arrancar con valores default\n");
                printf(" No se pudo editar BACKLOG, tomara el valor default BACKLOG = %d \n", *backlog);
                printf(" No se pudo editar PROCESS, tomara el valor default PROCESS = %d \n", *process);
                printf(" No se pudo editar ACQTIME, tomara el valor default ACQTIME = %d \n", *acqtime);
                printf(" No se pudo editar AVERAGE, tomara el valor default AVERAGE = %d \n", *average);
                custom=0;       /* Uso config por default */
                break;
            default:
                perror("open");
                exit(1);
        }
    }
    else
        custom=1;

    ffd=fdopen(fd, "r");

    if (custom)
    {
        /////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////// Busco a lo largo del archivo BACKLOG //////////////////////////////
        /////////////////////////////////////////////////////////////////////////////////////////////////
        change=0;
        while ((read = getline(&line, &len, ffd)) != -1)
        {
            switch (sscanf(line, "backlog = %s\n", sparam))
            {
                case EOF:       // Error
                    perror("sscanf");
                    exit(1);
                    break;
                case 0:         // No encontro
                    break;
                default:        // Encontro
                    change=1;
                    *backlog=atoi(sparam);
                    break;
            }
        }   
        if (change)
            printf(" Se edito BACKLOG = %d \n", *backlog);
        else
            printf(" No se pudo editar BACKLOG, tomara el valor default BACKLOG = %d \n", *backlog);
        rewind(ffd);
        /////////////////////////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////////////////////////
        
        /////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////// Busco a lo largo del archivo PROCESS //////////////////////////////
        /////////////////////////////////////////////////////////////////////////////////////////////////
        change=0;
        while ((read = getline(&line, &len, ffd)) != -1)
        {
            switch (sscanf(line, "process = %s\n", sparam))
            {
                case EOF:       // Error
                    perror("sscanf");
                    exit(1);
                    break;
                case 0:         // No encontro
                    break;
                default:        // Encontro
                    change=1;
                    *process=atoi(sparam);
                    break;
            }
        }   
        if (change)
            printf(" Se edito PROCESS = %d \n", *process);
        else
            printf(" No se pudo editar PROCESS, tomara el valor default PROCESS = %d \n", *process);
        rewind(ffd);
        /////////////////////////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////////////////////////

        /////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////// Busco a lo largo del archivo ACQTIME //////////////////////////////
        /////////////////////////////////////////////////////////////////////////////////////////////////
        change=0;
        while ((read = getline(&line, &len, ffd)) != -1)
        {
            switch (sscanf(line, "acq_time = %s\n", sparam))
            {
                case EOF:       // Error
                    perror("sscanf");
                    exit(1);
                    break;
                case 0:         // No encontro
                    break;
                default:        // Encontro
                    change=1;
                    *acqtime=atoi(sparam);
                    break;
            }
        }   
        if (change)
            printf(" Se edito ACQTIME = %d \n", *acqtime);
        else
            printf(" No se pudo editar ACQTIME, tomara el valor default ACQTIME = %d \n", *acqtime);
        rewind(ffd);
        /////////////////////////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////////////////////////

        /////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////// Busco a lo largo del archivo AVERAGE //////////////////////////////
        /////////////////////////////////////////////////////////////////////////////////////////////////
        change=0;
        while ((read = getline(&line, &len, ffd)) != -1)
        {
            switch (sscanf(line, "average = %s\n", sparam))
            {
                case EOF:       // Error
                    perror("sscanf");
                    exit(1);
                    break;
                case 0:         // No encontro
                    break;
                default:        // Encontro
                    change=1;
                    *average=atoi(sparam);
                    break;
            }
        }   
        if (change)
            printf(" Se edito AVERAGE = %d \n", *average);
        else
            printf(" No se pudo editar AVERAGE, tomara el valor default AVERAGE = %d \n", *average);
        rewind(ffd);
        /////////////////////////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////////////////////////
    }
    close(fd);
    return;
}
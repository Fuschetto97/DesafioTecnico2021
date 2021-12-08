#include "./Funciones/func.h"

int saveSTATE(FILE *fdd_State, long long int rightSensor, long long int centerSensor, long long int leftSensor, float heading, 
float revoluciones_rpm_s1, float revoluciones_rpm_s2, float distance_cm_s1, float distance_cm_s2)
{
    ssize_t loffset, lread;
    char * line = NULL;
    size_t len = 0;  
    int lineNro;
    char aux[10];

    //printf("22222 Distancia = Left: %lld cms, Center: %lld cms, Right: %lld cms \n", leftSensor, centerSensor, rightSensor);

    /* Busco a lo largo del archivo "Sensores, rightSensor =" */
    lineNro = 0;
    loffset = 0;
    fseek(fdd_State, 0, SEEK_SET);
    while ( (lread=getline(&line, &len, fdd_State )) != -1)
    {
        lineNro++;
        loffset = loffset + lread;
        //printf("lineNro: %d \n", lineNro);
        //printf("line %s \n", line); 
        switch (sscanf(line, "Sensores, rightSensor = %s\n", aux ))
        {
            case EOF:       // Error
                perror("sscanf");
                exit(1);
                break;
            case 0:         // No encontro
                //printf("No se encontro la linea: Sensores, rightSensor \n");
                break;
            default:        // Encontro
                //printf("La linea es la nro: %d, offset: %d \n", lineNro, loffset); 
                //printf("line %s \n", line); 
                sprintf(line, "Sensores, rightSensor = %03lld", rightSensor);
                fseek(fdd_State, (loffset-lread), SEEK_SET);
                if ( ( fwrite(line, sizeof(char), strlen(line), fdd_State)) != strlen(line))
                {
                    printf("Error escribiendo\n");
                    return -1;
                }
                fseek(fdd_State, (loffset), SEEK_SET);
                break;
        }

        switch (sscanf(line, "Sensores, centerSensor = %s\n", aux ))
        {
            case EOF:       // Error
                perror("sscanf");
                exit(1);
                break;
            case 0:         // No encontro
                break;
            default:        // Encontro
                sprintf(line, "Sensores, centerSensor = %03lld", centerSensor); 
                fseek(fdd_State, (loffset-lread), SEEK_SET);
                if ( ( fwrite(line, sizeof(char), strlen(line), fdd_State)) != strlen(line))
                {
                    printf("Error escribiendo\n");
                    return -1;
                }
                fseek(fdd_State, (loffset), SEEK_SET);
                break;
        }

        switch (sscanf(line, "Sensores, leftSensor = %s\n", aux ))
        {
            case EOF:       // Error
                perror("sscanf");
                exit(1);
                break;
            case 0:         // No encontro
                break;
            default:        // Encontro
                sprintf(line, "Sensores, leftSensor = %03lld", leftSensor); 
                fseek(fdd_State, (loffset-lread), SEEK_SET);
                if ( ( fwrite(line, sizeof(char), strlen(line), fdd_State)) != strlen(line))
                {
                    printf("Error escribiendo\n");
                    return -1;
                }
                fseek(fdd_State, (loffset), SEEK_SET);
                break;
        }

        switch (sscanf(line, "Brujula, Angulo = %s", aux ))
        {
            case EOF:       // Error
                perror("sscanf");
                exit(1);
                break;
            case 0:         // No encontro
                //printf("No se encontro: Brujula, Angulo\n");
                break;
            default:        // Encontro
                sprintf(line, "Brujula, Angulo = %.2f", heading); 
                fseek(fdd_State, (loffset-lread), SEEK_SET);
                if ( ( fwrite(line, sizeof(char), strlen(line), fdd_State)) != strlen(line))
                {
                    printf("Error escribiendo\n");
                    return -1;
                }
                fseek(fdd_State, (loffset), SEEK_SET);
                break;
        }

        switch (sscanf(line, "Encoder, Rev = %s - %s", aux, aux ))
        {
            case EOF:       // Error
                perror("sscanf");
                exit(1);
                break;
            case 0:         // No encontro
                //printf("No se encontro: Brujula, Angulo\n");
                break;
            default:        // Encontro
                sprintf(line, "Encoder, Rev = %f - %f", revoluciones_rpm_s1, revoluciones_rpm_s2); 
                fseek(fdd_State, (loffset-lread), SEEK_SET);
                if ( ( fwrite(line, sizeof(char), strlen(line), fdd_State)) != strlen(line))
                {
                    printf("Error escribiendo\n");
                    return -1;
                }
                fseek(fdd_State, (loffset), SEEK_SET);
                break;
        }

        switch (sscanf(line, "Encoder, Distancia = %s - %s", aux, aux ))
        {
            case EOF:       // Error
                perror("sscanf");
                exit(1);
                break;
            case 0:         // No encontro
                //printf("No se encontro: Brujula, Angulo\n");
                break;
            default:        // Encontro
                sprintf(line, "Encoder, Distancia = %d - %d", (unsigned int)distance_cm_s1, (unsigned int)distance_cm_s2); 
                fseek(fdd_State, (loffset-lread), SEEK_SET);
                if ( ( fwrite(line, sizeof(char), strlen(line), fdd_State)) != strlen(line))
                {
                    printf("Error escribiendo\n");
                    return -1;
                }
                fseek(fdd_State, (loffset), SEEK_SET);
                break;
        }
    }  

    return 0;
}

int main (int argc, char *argv[])
{
    /* state file */
        FILE* fdd_State;

    /* Variables sensores de distancia*/
        int fd_MIOgpio_PS;
        long long int rb_MIOgpio_PS[BYTE2READ_MIOgpio_PS];
        long long int rightSensor=-1, centerSensor=-1, leftSensor=-1;

    /* Variables brujula */
        int fd_brujula;                 // File descriptor para brujula
        int addr_brujula = 0x0D;        // The I2C address brujula
        float heading=0;                // Angulo absoluto al norte
        int xlow  = -12391;             // Calibracion
        int xhigh =   1301;
        int ylow  =  -6210;
        int yhigh =     75;   

    /* Variables encoder */
        int fd_encoder_1, fd_encoder_2;
        __s64 rb_encoder_1[BYTE2READ_encoder], rb_encoder_2[BYTE2READ_encoder];
        float revoluciones_rpm_s1, revoluciones_rpm_s2;
        float distance_cm_s1, distance_cm_s2;

    /* Semaforo */
        key_t key;              
        int semid, fdp;
        struct sembuf sb;
        //union semun arg;

        sb.sem_num = 0;
        sb.sem_op = -1; /* set to allocate resource */
        sb.sem_flg = SEM_UNDO;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////// Inicializar semaforo  /////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if((fdp=open("SemFile", O_RDONLY | O_CREAT, 0777))==-1){
        perror(" Open");
        exit(1);
    }
    if((key = ftok("SemFile", 'E'))==-1){
        perror(" ftok ");
        close(fdp);
        exit(1);
    }
    if ((semid = initsem(key, 1)) == -1) {      /* Configura el semaforo */
        perror("initsem");
        close(fdp);
        exit(1);
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////   State TXT  //////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    if ( (fdd_State = fopen("/home/root/Tesis/Apps/state.txt", "r+")) == NULL)
    {
        printf("Error abriendo state.txt\n");
        return -1;
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////  Sensores de distancia  ///////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    if ( (fd_MIOgpio_PS = open("/dev/chardev_MIOgpio_PS", O_RDWR)) == -1)
    {
        printf("Error abriendo chardev_MIOgpio_PS\n");
        return -1;
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////  Brujula  //////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    if ( (fd_brujula = open("/dev/i2c-0", O_WRONLY)) == -1)
    {
        printf("Error abriendo/dev/HMC5883L\n");
        return -1;
    }
    if (ioctl(fd_brujula, I2C_SLAVE, addr_brujula) < 0) {
        printf("Error ioctl HMC5883L: %s\n", strerror(errno));
        return -1;
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////  Encoder  //////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    if ( (fd_encoder_1 = open("/dev/chardev_encoder_EMIOgpio_PL_1", O_RDWR)) == -1)
    {
        printf("Error abriendo chardev_encoder_EMIOgpio_PL\n");
        return -1;
    }
    if ( (fd_encoder_2 = open("/dev/chardev_encoder_EMIOgpio_PL_2", O_RDWR)) == -1)
    {
        printf("Error abriendo chardev_encoder_EMIOgpio_PL\n");
        return -1;
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////  Calibracion de la Brujula  ///////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    if (CALIBRACION_ENABLE){
        printf("HMC5883L: Calibracion... \n");
        if ( calibracion_HMC5883L(fd_brujula, &xlow, &xhigh, &ylow, &yhigh) == -1)
        {
            printf("HMC5883L: Fallo la Calibracion... \n");
            return -1;
        }
        printf("HMC5883L: La calibracion termino.\n");
        //printf("HMC5883L: xlow: %d, xhigh: %d, ylow: %d, yhigh: %d\n", xlow, xhigh, ylow, yhigh);
    }
    printf("Llegue3 \n" );
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////  Lecturas  //////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    while(1)
    {
        ///////////////////////////////////////////// Sensores ///////////////////////////////////////////////
        if ( read(fd_MIOgpio_PS, rb_MIOgpio_PS, BYTE2READ_MIOgpio_PS) == -1)
        {
            printf("Error leyendo chardev_MIOgpio_PS\n");
            return -1;
        }

        rightSensor  = to_distance_cm(rb_MIOgpio_PS[0]);
        centerSensor = to_distance_cm(rb_MIOgpio_PS[1]);
        leftSensor   = to_distance_cm(rb_MIOgpio_PS[2]);

        //printf("Distancia = Left: %lld cms, Center: %lld cms, Right: %lld cms \n", leftSensor, centerSensor, rightSensor);

        ///////////////////////////////////////////// Brujula ///////////////////////////////////////////////
        while (1){
            heading = get_headeing_degree(fd_brujula, xlow, xhigh, ylow, yhigh);
            if (heading == -1){   // hubo ov
                continue;
            }
            if (heading == -2){   // Data skipped
                continue;
            }
            if (heading == -3){   // Data no available
                usleep(10000); // Sleep 80 mseg
                continue;
            }
            //printf("heading: %f \n", heading);
            break;
        }

        ///////////////////////////////////////////// Encoders //////////////////////////////////////////////
        if ( ( read(fd_encoder_1, rb_encoder_1, BYTE2READ_encoder)) == -1)
        {
            printf("Error leyendo chardev_encoder_EMIOgpio_PL\n");
            return -1;
        }
        if ( ( read(fd_encoder_2, rb_encoder_2, BYTE2READ_encoder)) == -1)
        {
            printf("Error leyendo chardev_encoder_EMIOgpio_PL\n");
            return -1;
        }
        printf("Tiempo en useg: %lld - %lld \n", rb_encoder_1[0], rb_encoder_2[0] );
        printf("Ranuras: %lld - %lld \n", rb_encoder_1[1], rb_encoder_2[1] );
        revoluciones_rpm_s1 = get_revoluciones_rpm(rb_encoder_1[0]);
        revoluciones_rpm_s2 = get_revoluciones_rpm(rb_encoder_1[0]);
        printf("Revoluciones: %d - %d rpm \n", (int)revoluciones_rpm_s1, (int)revoluciones_rpm_s2 );
        distance_cm_s1 = get_distance_m(rb_encoder_1[1]);
        distance_cm_s2 = get_distance_m(rb_encoder_2[1]);
        printf("Distancia recorrida Total: %f - %f ms \n", distance_cm_s1, distance_cm_s2);

        //////////////////////////////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////  State.txt update  /////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////////////    
        //printf("Trying to lock...\n");
        sb.sem_op = -1;         /* Asignar recurso */
        if (semop(semid, &sb, 1) == -1) {           /* semop setea, chequea o limpia uno o varios semaforos */
            perror("semop");
            exit(1);
        }
        //printf("Locked.\n");

        saveSTATE(fdd_State, rightSensor, centerSensor, leftSensor, heading, revoluciones_rpm_s1, revoluciones_rpm_s2, distance_cm_s1, distance_cm_s2);

        sb.sem_op = 1;          /* Libera el recurso */
        if (semop(semid, &sb, 1) == -1) {
            perror("semop");
            exit(1);
        }
        //printf("Unlocked\n");

        usleep(200000);
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////  Close all  /////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////// 
    fclose(fdd_State);
    close(fd_MIOgpio_PS);
    close(fd_brujula);
    close(fd_encoder_1);
    close(fd_encoder_2);

    return 0;
}

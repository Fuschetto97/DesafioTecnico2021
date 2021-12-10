#include "Server.h"
#include <time.h>
#include <assert.h>

#define MAXREAD 200
#define BUCKET_SIZE 5

///////////////
#define BYTE2READ 30*4

struct bmp280_dev
{
  unsigned short dig_T1;
  signed short dig_T2, dig_T3;
  unsigned short dig_P1;
  signed short dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
  double t_fine;
  double pres_raw;
  double temp_raw;
  double pres;
  double temp;
};
struct bmp280_dev bmp280_dev = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

double bmp280_compensate_T_double(double adc_T)
{
  double var1, var2, T;
  var1 = (((double)adc_T)/16384.0 - ((double)bmp280_dev.dig_T1)/1024.0) * ((double)bmp280_dev.dig_T2);
  var2 = ((((double)adc_T)/131072.0 - ((double)bmp280_dev.dig_T1)/8192.0) *
  (((double)adc_T)/131072.0 - ((double)bmp280_dev.dig_T1)/8192.0)) * ((double)bmp280_dev.dig_T3);
  bmp280_dev.t_fine = (double)(var1 + var2);
  T = (var1 + var2) / 5120.0;
  return T;
}

double bmp280_compensate_P_double(double adc_P)
{
  double var1, var2, p;
  var1 = ((double)bmp280_dev.t_fine/2.0) - 64000.0;
  var2 = var1 * var1 * ((double)bmp280_dev.dig_P6) / 32768.0;
  var2 = var2 + var1 * ((double)bmp280_dev.dig_P5) * 2.0;
  var2 = (var2/4.0)+(((double)bmp280_dev.dig_P4) * 65536.0);
  var1 = (((double)bmp280_dev.dig_P3) * var1 * var1 / 524288.0 + ((double)bmp280_dev.dig_P2) * var1) / 524288.0;
  var1 = (1.0 + var1 / 32768.0)*((double)bmp280_dev.dig_P1);
  if (var1 == 0)
  {
    printf(" Se intento dividir por cero!!!\n"); 
    return -1;
  }
  p = 1048576.0 - (double)adc_P;
  p = (p - (var2 / 4096.0)) * 6250.0 / var1;
  var1 = ((double)bmp280_dev.dig_P9) * p * p / 2147483648.0;
  var2 = p * ((double)bmp280_dev.dig_P8) / 32768.0;
  p = p + (var1 + var2 + ((double)bmp280_dev.dig_P7)) / 16.0;
  return p;
}

int save_calib_param(int *ubuff, int len)
{
  if (len > BYTE2READ)
    return -1;
  bmp280_dev.dig_T1= (ubuff[7] << 8) | ubuff[6];
  bmp280_dev.dig_T2= (ubuff[9] << 8) | ubuff[8];
  bmp280_dev.dig_T3= (ubuff[11] << 8) | ubuff[10];
  bmp280_dev.dig_P1= (ubuff[13] << 8) | ubuff[12];
  bmp280_dev.dig_P2= (ubuff[15] << 8) | ubuff[14];
  bmp280_dev.dig_P3= (ubuff[17] << 8) | ubuff[16];
  bmp280_dev.dig_P4= (ubuff[19] << 8) | ubuff[18];
  bmp280_dev.dig_P5= (ubuff[21] << 8) | ubuff[20];
  bmp280_dev.dig_P6= (ubuff[23] << 8) | ubuff[22];
  bmp280_dev.dig_P7= (ubuff[25] << 8) | ubuff[24];
  bmp280_dev.dig_P8= (ubuff[27] << 8) | ubuff[26];
  bmp280_dev.dig_P9= (ubuff[29] << 8) | ubuff[28];
  return 0;
}

//////////////

extern volatile int f_update;
extern volatile int f_close;

void Server_Reader_Child(int *backlog, int *process, int *acqtime, int *average, int *size, int buffer_size, char *buffer, int semid, int shmid, struct sembuf *sb, int fdp)
{
    int fd=0;
    int ubuff[BYTE2READ];
    int r;

    fd_set rfds;            // para select
    int selret, n;                      

    float *T_data=NULL;
    float *P_data=NULL;          // buffer de muestras
    double T_average_samples;       // muestra de temperatura
    double P_average_samples;       // muestra de presion
    int data_size = DEFAULT_AVERAGE;    // tamanio de buffer de muestras
    int data_count=0;

    time_t t;
    struct tm tm;
    FILE *fd_historico;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////// Reestaurar Signals //////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (signal(SIGCHLD, SIG_DFL) == SIG_ERR){           // Solo limpio la sigchld
        perror("signal");
        exit(1);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////  Inicializar historico  ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if ((fd_historico=fopen("sarx.txt", "w+")) == NULL)
    {
        perror("open");
        exit(1);
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////// Inicializar disp de adq ///////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if ( (fd = open("/dev/prueba-td3", O_RDWR)) == -1)
    {
        //perror("open");
        printf("Error abriendo prueba-td3\n");
        exit(1);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////// Inicializar buffer de muestras ////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    T_data = malloc(sizeof(int)*(data_size+1)); // allocate 5 int + /0 (default average)
    if(T_data==NULL){
        perror("malloc: No se consiguio memoria");
        close(fd);
        //close(fdh);
        exit(1);
    }
    P_data = malloc(sizeof(int)*(data_size+1)); // allocate 5 int + /0 (default average)
    if(T_data==NULL){
        perror("malloc: No se consiguio memoria");
        close(fd);
        //close(fdh);
        exit(1);
    }
    //memset(data, 0, sizeof(data));  // vacio el buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////// Bucle ///////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    while(1)
    {
        FD_ZERO(&rfds);
        printf(" CHILD READER DRIVER: El file descriptor de mi driver es: %d\n", fd);
        FD_SET(fd, &rfds);     // listener

        n=fd;   // unico

        if(f_close)
        {
            printf(" SERVER: La signal fue SIGINT \n");
            shmdt(buffer);      // Cuando el hijo es creado hereda todo lo de su padre, lo cierro antes de cerrar al hijo
            shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
            close(fdp);
            semctl(semid, 0, IPC_RMID);  //Elimino semaforo
            f_update=0;
            exit(0);
        }


        printf(" CHILD READER DRIVER: Antes del select\n");
        selret=select(n+1, &rfds, NULL, NULL, NULL);
        printf(" CHILD READER DRIVER: Despues del select\n");

        if (selret == -1)
        {                  
            if (errno != EINTR)
            {
                printf("error de select");
                close(fd);
                exit(1);
            }
            else 
            {
                printf(" CHILD READER DRIVER: A signal was caught\n");
                if(f_update)
                {  //Actualizacion de cond. de operacion 
                    printf(" CHILD READER DRIVER: La signal fue SIGUSR1 \n");
                    printf(" CHILD READER DRIVER: Lectura de archivo config... \n");
                    Config(backlog, process, acqtime, average);
                    f_update=0;
                    printf(" CHILD READER DRIVER: Lectura de archivo config terminada\n");
                }
                //if(f_close)
                //{
                //    printf(" SERVER: La signal fue SIGINT \n");
                //    shmdt(buffer);      // Cuando el hijo es creado hereda todo lo de su padre, lo cierro antes de cerrar al hijo
                //    shmctl(shmid, IPC_RMID, NULL);  //Elimino shared memory
                //    close(fdp);
                //    semctl(semid, 0, IPC_RMID);  //Elimino semaforo
                //    f_update=0;
                //    exit(0);
                //}
            }
        }
        else{
            if(FD_ISSET(fd, &rfds))
            {
                if ( (r = read(fd, ubuff, BYTE2READ)) == -1)
                {
                    //perror("close"):
                    printf("Error leyendo prueba-td3\n");
                    close(fd);
                    exit(1);
                }
                if (save_calib_param(ubuff, BYTE2READ) == -1)
                {
                    printf("Error en save_calib_param\n");
                    close(fd);
                    exit(1);
                }

                bmp280_dev.pres_raw = (((double)ubuff[0] * 65536) + ((double)ubuff[1] * 256) + (double)(ubuff[2] & 0xF0)) / 16;
                bmp280_dev.temp_raw = (((double)ubuff[3] * 65536) + ((double)ubuff[4] * 256) + (double)(ubuff[5] & 0xF0)) / 16;
                bmp280_dev.temp = bmp280_compensate_T_double(bmp280_dev.temp_raw);
                bmp280_dev.pres = bmp280_compensate_P_double(bmp280_dev.pres_raw);
                printf(" CHILD READER DRIVER: La temperatura en Celsius: %.2f C!\n", bmp280_dev.temp);
                printf(" CHILD READER DRIVER: La presion es: %.2f hPa\n", bmp280_dev.pres);

                printf(" CHILD READER DRIVER: Procesamiento de muestra...\n");
                if (data_count == *average){
                    printf(" CHILD READER DRIVER: Procedo a escribir la shm mem...\n"); /* escribo la media movil en recurso compartido */
                    printf("Trying to lock...\n");
                    sb->sem_op = -1;         /* Asignar recurso */
                    if (semop(semid, sb, 1) == -1) {           /* semop setea, chequea o limpia uno o varios semaforos */
                        perror("semop");
                        exit(1);
                    }
                    printf("Locked.\n");
                    T_average_samples = T_average_samples / *average;
                    P_average_samples = P_average_samples / *average;
                    *size=snprintf(buffer, buffer_size, "La temperatura es = %f", (T_average_samples));
                    if(*size<=0){
                        perror("snprintf");
                        close(fd);
                        exit(1);
                    }
                    *size=snprintf((buffer + *size), buffer_size, " La presion es = %f", (P_average_samples));
                    if(*size<=0){
                        perror("snprintf");
                        close(fd);
                        exit(1);
                    }
                    printf(" CHILD READER DRIVER: Buffer tiene %s\n", buffer);
                    printf(" CHILD READER DRIVER: Escritura de rec compartido terminada...\n");
                    sb->sem_op = 1;          /* Libera el recurso */
                    if (semop(semid, sb, 1) == -1) {
                        perror("semop");
                        exit(1);
                    }
                    printf("Unlocked\n");

                    printf("Escribo historico de T y P\n");
                    //date=system("date +%H:%M:%S");
                    t= time(NULL);
                    tm = *localtime(&t);
                    fflush(fd_historico);
                    fprintf (fd_historico, " %02d:%02d:%02d         %.2f  %.2f\n", tm.tm_hour, tm.tm_min, tm.tm_sec, T_average_samples, P_average_samples);
                    
                    data_count = 0;
                }
                if (data_count == data_size) {  // we're full up, so add a bucket
                    data_size += BUCKET_SIZE;
                    T_data = realloc(T_data, data_size * sizeof(float));
                    P_data = realloc(P_data, data_size * sizeof(float));
                }
                *(T_data+data_count)=bmp280_dev.temp;     // la agrego al buffer int data[average]
                *(P_data+data_count)=bmp280_dev.pres;
                *(P_data+data_count+1)='\0';
                *(T_data+data_count+1)='\0';
                T_average_samples=procesador_estadistico(T_data, *average);
                P_average_samples=procesador_estadistico(P_data, *average);
                data_count++;
                printf(" CHILD READER DRIVER: Procesamiento de muestra terminado...\n");
                printf(" CHILD READER DRIVER: La acumulacion de T es %f\n", T_average_samples);
                printf(" CHILD READER DRIVER: La acumulacion de P es %f\n", P_average_samples);
                if(sleep(*acqtime) != *acqtime){        // signal
                    if(f_update){  //Actualizacion de cond. de operacion 
                        printf(" CHILD READER DRIVER: La signal fue SIGUSR1 \n");
                        printf(" CHILD READER DRIVER: Lectura de archivo config... \n");
                        Config(backlog, process, acqtime, average);
                        f_update=0;
                        printf(" CHILD READER DRIVER: Lectura de archivo config terminada\n");
                    }
                }      
            }
        }
    }
    free(T_data);              
    close(fd);
    fclose(fd_historico);
    return;
}
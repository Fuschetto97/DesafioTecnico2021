#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

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
};
struct bmp280_dev bmp280_dev = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

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
//  pr_info(DEVICE_NAME": dig_T1 es: %d!\n", bmp280_dev.dig_T1);
  bmp280_dev.dig_T2= (ubuff[9] << 8) | ubuff[8];
//  pr_info(DEVICE_NAME": dig_T2 es: %d!\n", bmp280_dev.dig_T2);
  bmp280_dev.dig_T3= (ubuff[11] << 8) | ubuff[10];
//  pr_info(DEVICE_NAME": dig_T3 es: %d!\n", bmp280_dev.dig_T3);
  bmp280_dev.dig_P1= (ubuff[13] << 8) | ubuff[12];
//  pr_info(DEVICE_NAME": dig_P1 es: %d!\n", bmp280_dev.dig_P1);
  bmp280_dev.dig_P2= (ubuff[15] << 8) | ubuff[14];
//  pr_info(DEVICE_NAME": dig_P2 es: %d!\n", bmp280_dev.dig_P2);
  bmp280_dev.dig_P3= (ubuff[17] << 8) | ubuff[16];
//  pr_info(DEVICE_NAME": dig_P3 es: %d!\n", bmp280_dev.dig_P3);
  bmp280_dev.dig_P4= (ubuff[19] << 8) | ubuff[18];
//  pr_info(DEVICE_NAME": dig_P4 es: %d!\n", bmp280_dev.dig_P4);
  bmp280_dev.dig_P5= (ubuff[21] << 8) | ubuff[20];
//  pr_info(DEVICE_NAME": dig_P5 es: %d!\n", bmp280_dev.dig_P5);
  bmp280_dev.dig_P6= (ubuff[23] << 8) | ubuff[22];
//  pr_info(DEVICE_NAME": dig_P6 es: %d!\n", bmp280_dev.dig_P6);
  bmp280_dev.dig_P7= (ubuff[25] << 8) | ubuff[24];
//  pr_info(DEVICE_NAME": dig_P7 es: %d!\n", bmp280_dev.dig_P7);
  bmp280_dev.dig_P8= (ubuff[27] << 8) | ubuff[26];
//  pr_info(DEVICE_NAME": dig_P8 es: %d!\n", bmp280_dev.dig_P8);
  bmp280_dev.dig_P9= (ubuff[29] << 8) | ubuff[28];
//  pr_info(DEVICE_NAME": dig_P9 es: %d!\n", bmp280_dev.dig_P9);
  return 0;
}

int main (void)
{
  int fd;
  int ubuff[BYTE2READ];

  if ( (fd = open("/dev/prueba-td3", O_RDWR)) == -1)
  {
      //perror("open");
      printf("Error abriendo prueba-td3\n");
      return -1;
  }

  if ( ( read(fd, ubuff, BYTE2READ)) == -1)
  {
      //perror("close"):
      printf("Error leyendo prueba-td3\n");
      return -1;
  }
  //printf(" ubuff[0]: %.2f \n", (double)ubuff[0]);
  //printf(" ubuff[1]: %.2f \n", (double)ubuff[1]);

  if (save_calib_param(ubuff, BYTE2READ) == -1)
  {
    printf("Error en save_calib_param\n");
    return -1;
  }

  bmp280_dev.pres_raw = (((double)ubuff[0] * 65536) + ((double)ubuff[1] * 256) + (double)(ubuff[2] & 0xF0)) / 16;
  bmp280_dev.temp_raw = (((double)ubuff[3] * 65536) + ((double)ubuff[4] * 256) + (double)(ubuff[5] & 0xF0)) / 16;
  //printf(" La temperatura RAW: %.2f C!\n", bmp280_dev.temp_raw);
  //printf(" La presion RAW: %.2f hPa\n", bmp280_dev.pres_raw);



  bmp280_dev.temp_raw=bmp280_compensate_T_double(bmp280_dev.temp_raw);
  bmp280_dev.pres_raw=bmp280_compensate_P_double(bmp280_dev.pres_raw);

  printf(" La temperatura en Celsius: %.2f C!\n", bmp280_dev.temp_raw);
  printf(" La presion es: %.2f hPa\n", bmp280_dev.pres_raw);

  if ( (fd = close(fd)) == -1)
  {
      //perror("close"):
      printf("Error cerrando prueba-td3\n");
      return -1;
  }
  return 0;
}

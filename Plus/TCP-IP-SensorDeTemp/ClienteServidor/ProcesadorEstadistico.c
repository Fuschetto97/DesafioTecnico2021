#include "Server.h"


float procesador_estadistico(float *data, int average)
{
    int i=0;
    float average_samples=0;

    while(*(data+i) != '\0'){
        average_samples+=*(data+i);
        i++;
    }

    return (average_samples);
}
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    system("gnuplot -p gnuscript.gp");
    return 0;
}
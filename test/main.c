#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

int main(int argc, char const *argv[])
{
    const char *str = "123;hellsdf";
    char buffer[234] = {0};
    int code = 0;
    sscanf(str, "%d;%[^;]", &code, buffer);
    printf("buffer: [%s], code: %d\n", buffer, code);

    return 0;
}

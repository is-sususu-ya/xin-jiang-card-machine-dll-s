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
#ifdef ENABLE_TESTCODE

#include "SPM.h"

void onSPMEvent(HANDLE h, int code)
{
    char QRcode[64] = {0};
    switch (code)
    {
    case SPM_EVT_QRCODE:
        SPM_GetQrCode(h, QRcode);
        printf("Code:%s\r\n", QRcode);
        break;
    default:
        printf("Dvice Event:%d\r\n", code);
        break;
    }
}

int main(int argc, char const *argv[])
{
    HANDLE h = SPM_Create();
    if (SPM_Open(h, "/dev/ttyUSB0"))
    {
        SPM_SetCallBack(h, onSPMEvent);
        while (1)
        {
            sleep(1);
        }
    }
    return 0;
}
#endif
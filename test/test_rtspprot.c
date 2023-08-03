#include <stdio.h>
#include <string.h>
#include "proto/sio_rtspprot.h"

const char *resp = "DESCRIBE rtsp://127.0.0.1:8000/ RTSP/1.0\r\n"
                   "CSeq: 3\r\n"
                   "User-Agent: LibVLC/3.0.18 (LIVE555 Streaming Media v2016.11.28)\r\n"
                   "Accept: application/sdp\r\n"
                   "\r\n";

int main()
{
    struct sio_rtspprot *prot = sio_rtspprot_create();
    int l = sio_rtspprot_process(prot, resp, strlen(resp));
    printf("l: %d, len: %d\n", l, (int)strlen(resp));

    getc(stdin);

    return 0;
}
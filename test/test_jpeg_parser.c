#include <stdio.h>
#include <stdlib.h>
#include "moudle/rtsp/rtpack/sio_jpeg_parser.h"

int main()
{
    FILE *fp = fopen("test.jpg", "rb");
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    unsigned char *jpegdata = malloc(len);
    fread(jpegdata, 1, len, fp);

    fclose(fp);

    struct sio_jpeg_meta jpeg = { 0 };
    sio_jpeg_parser(&jpeg, jpegdata, len);

    printf("start: %02x %02x\n", jpeg.start[0], jpeg.start[1]);
    printf("end: %02x\n", jpeg.end[0]);
    printf("qtbl1: %02x %02x\n", jpeg.qtbl1[0], jpeg.qtbl1[1]);
    printf("qtbl2: %02x %02x\n", jpeg.qtbl2[0], jpeg.qtbl2[1]);
    printf("width: %d\n", jpeg.sof.width);
    printf("height: %d\n", jpeg.sof.height);

    getc(stdin);

    return 0;
}
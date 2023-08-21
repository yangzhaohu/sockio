#include "sio_avdev_jpeg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"

struct sio_avdev_jpeg
{
    struct sio_avinfo info;

    unsigned char *data;
    unsigned int length;
};

static inline
int sio_avdev_jpeg_open_jpeg(struct sio_avdev_jpeg *avdev, const char *name)
{
    FILE *fp = fopen(name, "rb");
    SIO_COND_CHECK_RETURN_VAL(!fp, -1);

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(len == 0, -1,
        fclose(fp));

    fseek(fp, 0, SEEK_SET);

    unsigned char *data = malloc(len);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!data, -1,
        fclose(fp));
    
    avdev->data = data;
    avdev->length = len;

    while (len > 0) {
        int l = fread(data, 1, len, fp);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(l == -1, -1,
            fclose(fp),
            free(data));

        len -= l;
    }

    fclose(fp);

    return 0;
}

sio_avdev_t sio_avdev_jpeg_open(const char *name)
{
    struct sio_avdev_jpeg *jpegdev = malloc(sizeof(struct sio_avdev_jpeg));
    SIO_COND_CHECK_RETURN_VAL(!jpegdev, NULL);

    int ret = sio_avdev_jpeg_open_jpeg(jpegdev, name);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
        free(jpegdev));

    jpegdev->info.encode = 26;
    jpegdev->info.fps = 25;
    
    return jpegdev;
}

int sio_avdev_jpeg_getinfo(sio_avdev_t avdev, struct sio_avinfo *info)
{
    struct sio_avdev_jpeg *jpegdev = (struct sio_avdev_jpeg *)avdev;
    *info = jpegdev->info;

    return 0;
}

int sio_avdev_jpeg_readframe(sio_avdev_t avdev, struct sio_avframe *frame)
{
    struct sio_avdev_jpeg *jpegdev = (struct sio_avdev_jpeg *)avdev;

    if (jpegdev->data == NULL || jpegdev->length == 0) {
        return -1;
    }

    frame->timestamp = 0;
    frame->data = jpegdev->data;
    frame->length = jpegdev->length;

    return 0;
}

int sio_avdev_jpeg_close(sio_avdev_t avdev)
{
    struct sio_avdev_jpeg *jpegdev = (struct sio_avdev_jpeg *)avdev;
    if (jpegdev->data) {
        free(jpegdev->data);
    }
    free(jpegdev);

    return 0;
}
#ifndef SIO_RTP_JPEG_H_
#define SIO_RTP_JPEG_H_

struct sio_rtp_jpeg;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_rtp_jpeg *sio_rtp_jpeg_create(unsigned int size);

int sio_rtp_jpeg_process(struct sio_rtp_jpeg *jpeg, const unsigned char *data, unsigned int len,
    void *handle, void (*payload)(void *handle, const unsigned char *data, unsigned int len));

int sio_rtp_jpeg_destory(struct sio_rtp_jpeg *jpeg);

#ifdef __cplusplus
}
#endif

#endif
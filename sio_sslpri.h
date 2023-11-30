#ifndef SIO_SSLPRI_H_
#define SIO_SSLPRI_H_

enum sio_sslopc
{
    /* set ca cert */
    SIO_SSL_CACERT,
    /* set user cert */
    SIO_SSL_USERCERT,
    /* set user private key */
    SIO_SSL_USERKEY,
};

union sio_sslopt
{
    const char *data;
};

#endif
#include "sio_sockssl.h"
#include <stdlib.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "sio_common.h"
#include "sio_log.h"

struct sio_sockssl
{
    SSL *ssl;
    BIO *rio;
    BIO *wio;
};

static inline
void sio_sockssl_errprint(const char *prefix, int err)
{
    unsigned long gerr = ERR_get_error();
    SIO_COND_CHECK_CALLOPS(gerr != 0 && err != SSL_ERROR_SSL,
        SIO_LOGE("%s err: %d, %s\n", prefix, err, ERR_error_string(gerr, NULL)));
}

struct sio_sockssl *sio_sockssl_create(sio_sslctx_t ctx)
{
    struct sio_sockssl *ssock = malloc(sizeof(struct sio_sockssl));
    SIO_COND_CHECK_RETURN_VAL(!ssock, NULL);

    memset(ssock, 0, sizeof(struct sio_sockssl));

    SSL *ssl = SSL_new(ctx);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!ssl, NULL,
        SIO_LOGE("SSL_new failed\n"),
        free(ssock));

    ssock->ssl = ssl;

    return ssock;
}

struct sio_sockssl *sio_sockssl_dup(struct sio_sockssl *ssock)
{
    struct sio_sockssl *dupsock = malloc(sizeof(struct sio_sockssl));
    SIO_COND_CHECK_RETURN_VAL(!dupsock, NULL);

    memset(dupsock, 0, sizeof(struct sio_sockssl));

    SSL *ssl = SSL_dup(ssock->ssl);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!ssl, NULL,
        SIO_LOGE("SSL_dup failed\n"),
        free(dupsock));

    dupsock->ssl = ssl;

    return dupsock;
}

int sio_sockssl_setopt(struct sio_sockssl *ssock, enum sio_sslopc cmd, union sio_sslopt *opt)
{
    int ret = 0;
    switch (cmd) {
    case SIO_SSL_USERCERT:
        ret = SSL_use_certificate_file(ssock->ssl, opt->data, SSL_FILETYPE_PEM);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 1, -1,
            SIO_LOGE("SSL_use_certificate_file %s\n", ERR_error_string(ERR_get_error(), NULL)));
        break;

    case SIO_SSL_USERKEY:
        ret = SSL_use_PrivateKey_file(ssock->ssl, opt->data, SSL_FILETYPE_PEM);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 1, -1,
            SIO_LOGE("SSL_use_PrivateKey_file %s\n", ERR_error_string(ERR_get_error(), NULL)));

        ret = SSL_check_private_key(ssock->ssl);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 1, -1,
            SIO_LOGE("SSL_check_private_key %s\n", ERR_error_string(ERR_get_error(), NULL)));
        break;
    case SIO_SSL_VERIFY_PEER:
    {
        int mode = opt->enable ? SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT : 0;
        SSL_set_verify(ssock->ssl, mode, NULL);
        break;
    }
    
    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_sockssl_setfd(struct sio_sockssl *ssock, sio_fd_t fd)
{
    int ret = SSL_set_fd(ssock->ssl, fd);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == 0, -1,
        SIO_LOGE("SSL_set_fd failed\n"));

    return 0;
}

int sio_sockssl_enable_membio(struct sio_sockssl *ssock)
{
    if (ssock->rio) {
        return 0;
    }

    ssock->rio = BIO_new(BIO_s_mem());
    ssock->wio = BIO_new(BIO_s_mem());
    SSL_set_bio(ssock->ssl, ssock->rio, ssock->wio);

    return 0;
}

int sio_sockssl_accept(struct sio_sockssl *ssock)
{
    int ret = SSL_accept(ssock->ssl);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 1, ret,
        ret = -SSL_get_error(ssock->ssl, ret),
        sio_sockssl_errprint("SSL_accept", -ret));

    return 0;
}

int sio_sockssl_connect(struct sio_sockssl *ssock)
{
    int ret = SSL_connect(ssock->ssl);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 1, ret,
        ret = -SSL_get_error(ssock->ssl, ret),
        sio_sockssl_errprint("SSL_connect", -ret));

    return 0;
}

int sio_sockssl_handshake(struct sio_sockssl *ssock)
{
    int ret = SSL_do_handshake(ssock->ssl);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 1, ret,
        ret = -SSL_get_error(ssock->ssl, ret),
        sio_sockssl_errprint("SSL_do_handshake", -ret));

    return 0;
}

int sio_sockssl_readfrom_wbio(struct sio_sockssl *ssock, char *buf, int len)
{
    int ret = BIO_read(ssock->wio, buf, len);
    SIO_COND_CHECK_RETURN_VAL(ret < 0, -1);
    return ret;
}

int sio_sockssl_wbio_pending(struct sio_sockssl *ssock)
{
    return BIO_ctrl_pending(ssock->wio);
}

int sio_sockssl_writeto_rbio(struct sio_sockssl *ssock, char *data, int len)
{
    int ret = BIO_write(ssock->rio, data, len);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret < 0, -1,
        sio_sockssl_errprint("BIO_write", ret));
    return ret;
}

int sio_sockssl_read(struct sio_sockssl *ssock, char *buf, int len)
{
    int ret = SSL_read(ssock->ssl, buf, len);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret <= 0, ret,
        ret = -SSL_get_error(ssock->ssl, ret),
        sio_sockssl_errprint("SSL_read", -ret));

    return ret;
}

int sio_sockssl_write(struct sio_sockssl *ssock, const char *data, int len)
{
    int ret = SSL_write(ssock->ssl, data, len);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret <= 0, ret,
        ret = -SSL_get_error(ssock->ssl, ret),
        sio_sockssl_errprint("SSL_write", -ret));

    return ret;
}

int sio_sockssl_shutdown(struct sio_sockssl *ssock)
{
    SSL_set_shutdown(ssock->ssl, SSL_SENT_SHUTDOWN);
    int ret = SSL_shutdown(ssock->ssl);
    SSL_clear(ssock->ssl);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret < 0, ret,
        ret = -SSL_get_error(ssock->ssl, ret),
        sio_sockssl_errprint("SSL_shutdown", -ret));

    SIO_COND_CHECK_RETURN_VAL(ret == 1, 0);
    return 0;
}

int sio_sockssl_close(struct sio_sockssl *ssock)
{
    SSL_clear(ssock->ssl);

    return 0;
}

int sio_sockssl_destory(struct sio_sockssl *ssock)
{
    // SSL_CTX_free(ssock->sslctx);
    SSL_free(ssock->ssl);
    free(ssock);

    return 0;
}
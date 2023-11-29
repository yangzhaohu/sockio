#include "sio_sockssl.h"
#include <stdlib.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/ssl2.h>
#include <openssl/ssl3.h>
#include <openssl/err.h>
#include "sio_common.h"
#include "sio_log.h"

struct sio_sockssl
{
    SSL_CTX *sslctx;
    SSL *ssl;
};

struct sio_sockssl *sio_sockssl_create()
{
    struct sio_sockssl *ssock = malloc(sizeof(struct sio_sockssl));
    SIO_COND_CHECK_RETURN_VAL(!ssock, NULL);

    memset(ssock, 0, sizeof(struct sio_sockssl));
    SSL_CTX *sslctx = SSL_CTX_new(SSLv23_method());
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!sslctx, NULL,
        SIO_LOGE("SSL_CTX_new failed\n"),
        free(ssock));

    // 双向验证
    // SSL_VERIFY_PEER---要求对证书进行认证，没有证书也会放行
    // SSL_VERIFY_FAIL_IF_NO_PEER_CERT---要求客户端需要提供证书，但验证发现单独使用没有证书也会放行
    // SSL_CTX_set_verify(sslctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    
    SSL *ssl = SSL_new(sslctx);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!ssl, NULL,
        SIO_LOGE("SSL_new failed\n"),
        SSL_CTX_free(sslctx),
        free(ssock));

    ssock->sslctx = sslctx;
    ssock->ssl = ssl;

    return ssock;
}

struct sio_sockssl *sio_sockssl_create2(sio_fd_t fd)
{
    return 0;
}

int sio_sockssl_setopt(struct sio_sockssl *ssock, enum sio_ssloptc cmd, union sio_sslopt *opt)
{
    int ret = 0;
    switch (cmd) {
    case SIO_SSL_CACERT:
        // SSL_CTX_load_verify_locations(ssock->sslctx, opt->data, NULL);
        break;

    case SIO_SSL_USERCERT:
        // ret = SSL_CTX_use_certificate_file(ssock->sslctx, opt->data, SSL_FILETYPE_PEM);
        SSL_use_certificate_file(ssock->ssl, opt->data, SSL_FILETYPE_PEM);
        break;

    case SIO_SSL_USERKEY:
        // ret = SSL_CTX_use_PrivateKey_file(ssock->sslctx, opt->data, SSL_FILETYPE_PEM);
        // ret = SSL_CTX_check_private_key(ssock->sslctx);
        SSL_use_PrivateKey_file(ssock->ssl, opt->data, SSL_FILETYPE_PEM);
        ret = SSL_check_private_key(ssock->ssl);
        break;
    
    default:
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

int sio_sockssl_accept(struct sio_sockssl *ssock)
{
    int ret = SSL_accept(ssock->ssl);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 1, ret,
        ret = -SSL_get_error(ssock->ssl, ret),
        SIO_LOGD("accept ret: %d, %s\n", -ret, ERR_error_string(ERR_get_error(), NULL)));

    return 0;
}

int sio_sockssl_connect(struct sio_sockssl *ssock)
{
    int ret = SSL_connect(ssock->ssl);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 1, ret,
        ret = -SSL_get_error(ssock->ssl, ret));

    return 0;
}

int sio_sockssl_handshake(struct sio_sockssl *ssock)
{
    int ret = SSL_do_handshake(ssock->ssl);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 1, ret,
        ret = -SSL_get_error(ssock->ssl, ret));

    return 0;
}

int sio_sockssl_read(struct sio_sockssl *ssock, char *buf, int len)
{
    int ret = SSL_read(ssock->ssl, buf, len);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret <= 0, ret,
        ret = -SSL_get_error(ssock->ssl, ret));

    return ret;
}

int sio_sockssl_write(struct sio_sockssl *ssock, const char *data, int len)
{
    int ret = SSL_write(ssock->ssl, data, len);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret <= 0, ret,
        ret = -SSL_get_error(ssock->ssl, ret));

    return ret;
}

int sio_sockssl_shutdown(struct sio_sockssl *ssock)
{
    SSL_set_shutdown(ssock->ssl, SSL_SENT_SHUTDOWN);
    int ret = SSL_shutdown(ssock->ssl);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret < 0, ret,
        ret = -SSL_get_error(ssock->ssl, ret));

    SIO_COND_CHECK_RETURN_VAL(ret == 1, 0);
    return 0;
}

int sio_sockssl_destory(struct sio_sockssl *ssock)
{
    SSL_CTX_free(ssock->sslctx);
    SSL_free(ssock->ssl);
    free(ssock);

    return 0;
}
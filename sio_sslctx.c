#include "sio_sslctx.h"
#include <stdlib.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "sio_common.h"
#include "sio_log.h"

sio_sslctx_t sio_sslctx_create()
{
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_method());
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!ctx, NULL,
        SIO_LOGE("SSL_CTX_new failed\n"));

    // SSL_CTX_set_verify(sslctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

    return ctx;
}

int sio_sslctx_setopt(sio_sslctx_t ctx, enum sio_sslopc cmd, union sio_sslopt *opt)
{
    int ret = 0;
    switch (cmd) {
    case SIO_SSL_CACERT:
        ret = SSL_CTX_load_verify_locations(ctx, opt->data, NULL);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 1, -1,
            SIO_LOGE("SSL_CTX_load_verify_locations", ERR_error_string(ERR_get_error(), NULL)));
        break;

    case SIO_SSL_USERCERT:
        ret = SSL_CTX_use_certificate_file(ctx, opt->data, SSL_FILETYPE_PEM);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 1, -1,
            SIO_LOGE("SSL_CTX_use_certificate_file", ERR_error_string(ERR_get_error(), NULL)));
        break;

    case SIO_SSL_USERKEY:
        ret = SSL_CTX_use_PrivateKey_file(ctx, opt->data, SSL_FILETYPE_PEM);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 1, -1,
            SIO_LOGE("SSL_CTX_use_PrivateKey_file", ERR_error_string(ERR_get_error(), NULL)));

        ret = SSL_CTX_check_private_key(ctx);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 1, -1,
            SIO_LOGE("SSL_CTX_check_private_key", ERR_error_string(ERR_get_error(), NULL)));
        break;
    
    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_sslctx_destory(sio_sslctx_t ctx)
{
    SSL_CTX_free(ctx);

    return 0;
}
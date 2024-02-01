#ifndef SIO_DLOPEN_H_
#define SIO_DLOPEN_H_

typedef void* sio_dl;
typedef const char* sio_dlsymd;
typedef void* sio_dlsym;

#ifdef __cplusplus
extern "C" {
#endif

sio_dl sio_dlopen(const char *file);

int sio_dlsyms(sio_dl dl, const sio_dlsymd symsd[], unsigned short num, sio_dlsym syms);

int sio_dlclose(sio_dl dl);

#ifdef __cplusplus
}
#endif

#endif
#include "sio_dlopen.h"
#include <string.h>

#ifdef WIN32
#include <Windows.h>
#define DL_HANDLE               HMODULE
#define DL_OPEN(pathname)       LoadLibraryA(pathname)
#define DL_CLOSE(handle)        FreeLibrary(handle)
#define DL_SYM(handle, symbol)  GetProcAddress(handle, symbol)
#define DL_ERROR                GetLastError()
#else
#include <dlfcn.h>
#define DL_HANDLE               void*
#define DL_OPEN(pathname)       dlopen(pathname, RTLD_LAZY)
#define DL_CLOSE(handle)        dlclose(handle)
#define DL_SYM(handle, symbol)  dlsym(handle, symbol)
#define DL_ERROR                dlerror()
#endif

typedef sio_dlsym* sio_dlsym_ref;

sio_dl sio_dlopen(const char *file)
{
    sio_dl dl = DL_OPEN(file);
    return dl;
}

int sio_dlsyms(sio_dl dl, const sio_dlsymd symsd[], unsigned short num, sio_dlsym syms)
{
    int count = 0;
    sio_dlsym_ref sym = (sio_dlsym_ref)syms;
    for (int i = 0; i < num; i++) {
        *sym = (sio_dlsym)DL_SYM((DL_HANDLE)dl, symsd[i]);
        *sym != NULL ? count++ : 0;
        sym++;
    }

    return count;
}

int sio_dlclose(sio_dl dl)
{
    int ret = DL_CLOSE((DL_HANDLE)dl);
    return ret;
}
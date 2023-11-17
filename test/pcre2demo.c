#include <stdio.h>
#include <string.h>
#include "pcre2/pcre2posix.h"
#include "sio_log.h"

// demo
/*
*  pattern   |   strval              |  result
*            |                       |
*  html      |   /html/index.html    |  match
*  index     |   /html/index.html    |  match
*  /index    |   /html/index.html    |  match
*  ^html     |   /html/index.html    |  no match
*  ^/html    |   /html/index.html    |  match
*  ^/html$   |   /html/index.html    |  no match
*  ^.*html$  |   /html/index.html    |  match
*/

int main(int argc, char **argv)
{
    if (argc < 3) {
        return -1;
    }

    SIO_LOGI("reg: %s, val: %s, %ld\n", argv[1], argv[2], strlen(argv[2]));
    const char* pattern = argv[1];
    const char* string = argv[2];
    
    regex_t regex;
    int ret = pcre2_regcomp(&regex, pattern, REG_EXTENDED);
    if (ret) {
        SIO_LOGI("pcre2 compiler regex failed\n");
        return -1;
    }

    ret = pcre2_regexec(&regex, string, 0, NULL, 0);
    if (ret != 0) {
        SIO_LOGI("no match\n");
        pcre2_regfree(&regex);
        return -1;
    }

    SIO_LOGI("match\n");
    pcre2_regfree(&regex);

    return 0;
}
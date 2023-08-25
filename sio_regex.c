#include "sio_regex.h"
#include "pcre2/pcre2posix.h"
#include "sio_log.h"

int sio_regex(const char *pattern, const char *string, int length)
{
    regex_t regex;
    int ret = pcre2_regcomp(&regex, pattern, REG_EXTENDED);
    if (ret) {
        SIO_LOGE("pcre2 compiler regex failed\n");
        return -1;
    }

    char *modify = (char *)string;
    char tmp = modify[length];
    modify[length] = 0;
    regmatch_t match[64] = { 0 };
    ret = pcre2_regexec(&regex, string, 64, match, 0);
    modify[length] = tmp;

    int matchpos = -1;
    if (ret == 0) {
        for (int i = 0; i < 64 && match[i].rm_so != -1; i++) {
            matchpos = match[i].rm_eo;
        }
    }

    pcre2_regfree(&regex);

    return matchpos;
}
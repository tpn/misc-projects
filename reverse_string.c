#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{
    static const char * const forward = "abcdefg\0";

    size_t sz = (size_t)strlen(forward);
    char *reverse = (char *)malloc(sz);
    memset(reverse, 0, sz);

    char *r = &reverse[sz-1];
    const char *f = &forward[0];

    while (*f)
        *r-- = *f++;

    printf("forward: %s\nreverse: %s\n", forward, reverse);

    return 0;

}

/* vim:set ts=8 sw=4 sts=4 et: */

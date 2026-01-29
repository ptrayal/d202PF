/* ************************************************************************
*  file:  asciipasswd.c (derived from mudpasswd.c)    Part of CircleMud   *
*  Usage: generating hashed passwords for an ascii playerfile             *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*  All Rights Reserved                                                    *
************************************************************************* */


#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <stddef.h>

/* Safely capitalize first character */
static void cap_first(const char *src, char *dst, size_t dst_size)
{
    if (!src || !dst || dst_size == 0) return;

    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';

    if (dst[0])
        dst[0] = UPPER(dst[0]);
}

/* Prevent compiler from optimizing memory wipe */
static void secure_zero(void *ptr, size_t len)
{
    volatile unsigned char *p = ptr;
    while (len--) {
        *p++ = 0;
    }
}

int main(int argc, char **argv)
{
    char name_buf[256];
    char pass_buf[256];
    char *hashed;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <name> <password>\n", argv[0]);
        return 1;
    }

    cap_first(argv[1], name_buf, sizeof(name_buf));

    /* Copy password into writable memory */
    strncpy(pass_buf, argv[2], sizeof(pass_buf) - 1);
    pass_buf[sizeof(pass_buf) - 1] = '\0';

    hashed = CRYPT(pass_buf, name_buf);

    printf("Name: %s\n", name_buf);
    printf("Pass: %s\n", hashed);

    /* Wipe sensitive data */
    secure_zero(pass_buf, sizeof(pass_buf));

    return 0;
}
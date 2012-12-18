/*
 * Copyright (c) 2012 Mark Heily <mark@heily.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <Block.h>

typedef int (^IntBlock)();

IntBlock incrementor(int start) 
{
    __block int i = start;

    return Block_copy( ^{
            return ++i;
            }
            );
}

int 
main(int argc, char **argv)
{
    IntBlock counter = incrementor(0);
    int rv;

    printf("increment test... ");
    rv = counter();
    rv = counter();
    rv = counter();
    Block_release(counter);
    if (rv == 3) {
        puts("ok");
        exit(0);
    } else {
        puts("ERROR");
        exit(1);
    }
}

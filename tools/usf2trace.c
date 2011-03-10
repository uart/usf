/*
 * Copyright (C) 2009-2011, David Eklov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <uart/usf.h>

#define E(err, str) do {                                        \
    if ((err) != USF_ERROR_OK)                                  \
        print_and_exit("%s:%d: %s\n",                           \
                __FUNCTION__, __LINE__, usf_strerror(err));     \
} while (0)

typedef struct {
    char *i_file_name;
    char *o_file_name;
} args_t;

static char *usage_str = "Usage: usf2trc [INFILE] [OUTFILE]";

static void __attribute__ ((format (printf, 1, 2)))
print_and_exit(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    exit(EXIT_FAILURE);
}

static void
parse_args(args_t *args, int argc, char **argv)
{
    args->i_file_name = NULL;
    args->o_file_name = NULL;

    if (argc > 1)
        args->i_file_name = argv[1];

    if (argc > 2)
        args->o_file_name = argv[2];

    if (argc > 3)
        print_and_exit("%s\n", usage_str);
}

int
main(int argc, char **argv)
{
    args_t args;
    usf_file_t *usf_i_file;
    usf_file_t *usf_o_file;
    usf_header_t *i_header;
    usf_header_t o_header;
    usf_event_t event1;
    usf_event_t event2;
    usf_error_t error;

    parse_args(&args, argc, argv);

    error = usf_open(&usf_i_file, args.i_file_name);
    E(error, "usf_open");

    error = usf_header((const usf_header_t **)&i_header, usf_i_file);
    E(error, "usf_header");

    memcpy(&o_header, i_header, sizeof(usf_header_t));
    o_header.flags &= ~USF_FLAG_BURST;
    o_header.flags |= USF_FLAG_TRACE;

    error = usf_create(&usf_o_file, args.o_file_name, &o_header);
    E(error, "usf_create");
    
    while ((error = usf_read(usf_i_file, &event1)) == USF_ERROR_OK) {
        usf_access_t *pc1;

        switch (event1.type) {
            case USF_EVENT_SAMPLE:
                pc1 = &event1.u.sample.begin;
                break;
            case USF_EVENT_DANGLING:
                pc1 = &event1.u.dangling.begin;
                break;
            case USF_EVENT_BURST:
                continue;
            case USF_EVENT_TRACE:
                assert(0);
        }

        event2.type = USF_EVENT_TRACE;
        event2.u.trace.access = *pc1;
        
        error = usf_append(usf_o_file, &event2);
        E(error, "usf_append");
    }

    if (error != USF_ERROR_EOF)
        E(error, "usf_read");

    usf_close(usf_i_file);
    usf_close(usf_o_file);
    return 0;
}

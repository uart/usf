/*
 * Copyright (C) 2010-2011, David Eklov
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

static char *usage_str = "Usage: usfsplit [INFILE] [OUTFILE]";

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
    usf_file_t *usf_o_file0;
    usf_file_t *usf_o_file1;
    usf_header_t *i_header;
    usf_header_t o_header;
    usf_event_t event;
    usf_error_t error;
    char file_name[256];

    parse_args(&args, argc, argv);

    error = usf_open(&usf_i_file, args.i_file_name);
    E(error, "usf_open");

    error = usf_header((const usf_header_t **)&i_header, usf_i_file);
    E(error, "usf_header");

    memcpy(&o_header, i_header, sizeof(usf_header_t));
    o_header.flags &= ~USF_FLAG_BURST;

    snprintf(file_name, 256, "%s.0", args.o_file_name);
    error = usf_create(&usf_o_file0, file_name, &o_header);
    E(error, "usf_create");
    
    snprintf(file_name, 256, "%s.1", args.o_file_name);
    error = usf_create(&usf_o_file1, file_name, &o_header);
    E(error, "usf_create");
    
    while ((error = usf_read(usf_i_file, &event)) == USF_ERROR_OK) {
        usf_tid_t tid;

        switch (event.type) {
            case USF_EVENT_SAMPLE:
                tid = event.u.sample.begin.tid;
                break;
            case USF_EVENT_DANGLING:
                tid = event.u.dangling.begin.tid;
                break;
            case USF_EVENT_BURST:
                continue;
            case USF_EVENT_TRACE:
                assert(0);
        }

        switch (tid) {
            case 0:
                error = usf_append(usf_o_file0, &event);
                E(error, "usf_append");
                break;
            case 1:
                error = usf_append(usf_o_file1, &event);
                E(error, "usf_append");
                break;
            default:
                assert(0);
        }
    }

    if (error != USF_ERROR_EOF)
        E(error, "usf_read");

    usf_close(usf_i_file);
    usf_close(usf_o_file0);
    usf_close(usf_o_file1);
    return 0;
}

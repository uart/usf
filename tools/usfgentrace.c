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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <uart/usf.h>

#define MAX_NR_SDIST 256

#define USF_ERROR(_e) do {              \
    usf_error_t e = (_e);               \
    if ((e) != USF_ERROR_OK) {          \
        fprintf(stderr, "Error: %s\n", usf_strerror(e)); \
        exit(EXIT_FAILURE);             \
    }                                   \
} while (0)
        

static char *usage_str = "Usage: usfgentrace [OPTS]... [STEP] [ITER] [SIZE]...\n";

static int   args_step;
static long  args_iter;
static long  args_sdist[MAX_NR_SDIST];
static int   args_sdist_len;
static char *args_file_name;

static long size[MAX_NR_SDIST];
static long size_len;

static usf_header_t header = {
    .version = USF_VERSION_CURRENT,
    .compression = USF_COMPRESSION_BZIP2,
    .flags = USF_FLAG_NATIVE_ENDIAN | USF_FLAG_TRACE,
};

static void
compute_size()
{
    int i;
    double size_sum = 0;

    size_len = args_sdist_len;
    for (i = 0; i < size_len; i++) {
        double s =  (args_sdist[i] - size_sum) / (size_len - i);
        size[i] = s;
        size_sum += s;
    }
}

static void
write_usf()
{
    long i, j, k;
    usf_file_t *file;
    usf_event_t event;
    usf_atime_t time = 0;

    memset(&event, 0, sizeof event);
    event.type = USF_EVENT_TRACE;
    event.u.trace.access.len = 1;

    USF_ERROR(usf_create(&file, args_file_name, &header));
    for (i = 0; i < args_iter; i++) {
        long size_max = size[size_len - 1];
        for (j = 0; j < size[size_len - 1]; j += args_step)
            for (k = 0; k < size_len; k++) {
                event.u.trace.access.time = time++;
                event.u.trace.access.addr = k * size_max + j % size[k];
                USF_ERROR(usf_append(file, &event));
            }
    }
    USF_ERROR(usf_close(file));
}

static void
parse_args(int argc, char **argv)
{
    int i, j, c;

    while ((c = getopt(argc, argv, "ho:")) != -1) {
        switch (c) {
            case 'h':
                fprintf(stderr, "%s", usage_str);
                exit(EXIT_SUCCESS);
                break;
            case 'o':
                args_file_name = optarg;
                break;
        }
    }

    if (argc - optind < 3) {
        fprintf(stderr, "%s", usage_str);
        exit(EXIT_FAILURE);
    }

    args_step = strtol(argv[optind + 0], NULL, 10);
    args_iter = strtol(argv[optind + 1], NULL, 10);
    for (i = optind + 2, j = 0; i < argc && j < MAX_NR_SDIST; i++, j++)
        args_sdist[j] = strtol(argv[i], NULL, 10);
    args_sdist_len = j;
}

int
main(int argc, char **argv)
{
    int i;
    parse_args(argc, argv);
    header.argc = argc;
    header.argv = argv;
    compute_size();
    write_usf();
    return 0;
}

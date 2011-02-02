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
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <getopt.h>
#include <uart/usf.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define E_ERROR(fmt, args...)   \
    print_and_exit("%s:%s " fmt, __FILE__, __LINE__, args)

#define E_USF(e, name) do {                                     \
    usf_error_t _e = (e);                                       \
    if (_e != USF_ERROR_OK)                                     \
        E_ERROR("%s: %s\n", name, usf_strerror(_e));            \
} while (0)

#define E_NULL(e, name) do {                                    \
    __typeof__(e) _e = (e);                                     \
    if (!_e)                                                    \
        E_ERROR("%s: %s\n", name, strerror(errno));             \
} while (0)


static char *usage_str = "Usage: usfcat [OPTION]... [FILE]...\n"
                         "Concatenate FILE(s) to standard output.\n\n"
                         "  -h, --help\t\tdisplay this help and exit\n"
                         "  -c, --compression\tSet compression algorithm\n";

typedef struct {
    int    ifile_list_len;
    char **ifile_list;

    usf_compression_t compression;
} args_t;

static void
print_and_exit(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    exit(EXIT_FAILURE);
}

static void
parse_args(args_t *args, int argc, char **argv)
{
    int c;
    int not_used;

    static struct option long_opts[] = {
        {"help", no_argument, NULL, 'h'},
        {"compression", required_argument, NULL, 'c'},
    };

    args->compression = (usf_compression_t)-1;

    while ((c = getopt_long(argc, argv, "h", long_opts, &not_used)) != -1) {
        switch (c) {
            case 'c':
                if (!strcmp("none", optarg))
                    args->compression = USF_COMPRESSION_NONE;
                else if (!strcmp("bzip2", optarg))
                    args->compression = USF_COMPRESSION_BZIP2;
                else
                    print_and_exit("Unknown compression\n\n%s\n", usage_str);
                break;
            default:
                print_and_exit(usage_str);
                break;
        }
    }

    if (optind < argc) {
        args->ifile_list = &argv[optind];
        args->ifile_list_len = argc - optind;
    } else 
        print_and_exit(usage_str);
}

static usf_file_t **
open_infiles(char **ifile_list, int ifile_list_len)
{
    usf_error_t error;
    usf_file_t **usf_ifile_list;

    usf_ifile_list = 
        (usf_file_t **)malloc(sizeof(usf_file_t *) * ifile_list_len);
    E_NULL(usf_ifile_list, "malloc");

    for (int i = 0; i < ifile_list_len; i++) {
        error = usf_open(&usf_ifile_list[i], ifile_list[i]);
        E_USF(error, "usf_open");
    }

    return usf_ifile_list;
}

static void
make_header(usf_file_t    **usf_ifile_list,
            int             usf_ifile_list_len,
            usf_header_t   *outheader)
{
    usf_error_t error;

    outheader->version = USF_VERSION_CURRENT;
    outheader->flags = USF_FLAG_NATIVE_ENDIAN;
    outheader->time_begin = (usf_wtime_t)-1;
    outheader->time_end = 0;
    outheader->line_sizes = 0;
    outheader->argc = 0;
    outheader->argv = NULL;

    for (int i = 0; i < usf_ifile_list_len; i++) {
        const usf_header_t *header;

        error = usf_header(&header, usf_ifile_list[i]);
        E_USF(error, "usf_header");

        outheader->flags |= header->flags;
        outheader->time_begin = MIN(header->time_begin, outheader->time_begin);
        outheader->time_end = MAX(header->time_end, outheader->time_end);
        outheader->line_sizes |= header->line_sizes;
    }

    outheader->flags &= ~USF_FLAG_FOREIGN_ENDIAN;
    outheader->flags &= ~USF_FLAG_DELTA;
}

static void
close_infiles(usf_file_t **usf_ifile_list, int usf_ifile_list_len)
{
    usf_error_t error;

    for (int i = 0; i < usf_ifile_list_len; i++) {
        error = usf_close(usf_ifile_list[i]);
        E_USF(error, "usf_close");
    }
}

int
main(int argc, char **argv)
{
    args_t args;
    usf_file_t *usf_ofile;
    usf_file_t **usf_ifile_list;
    int usf_ifile_list_len;
    usf_header_t header;
    usf_error_t error;

    parse_args(&args, argc, argv);

    usf_ifile_list_len = args.ifile_list_len;
    usf_ifile_list = open_infiles(args.ifile_list, usf_ifile_list_len);

    make_header(usf_ifile_list, usf_ifile_list_len, &header);
    if (args.compression != (usf_compression_t)-1)
        header.compression = args.compression;

    error = usf_create(&usf_ofile, NULL, &header);
    E_USF(error, "usf_create");

    for (int i = 0; i < usf_ifile_list_len; i++) {
        usf_event_t event;
        while (usf_read(usf_ifile_list[i], &event) == USF_ERROR_OK) {
            error = usf_append(usf_ofile, &event);
            E_USF(error, "usf_append");
        }
    }

    error = usf_close(usf_ofile);
    E_USF(error, "usf_close");
    close_infiles(usf_ifile_list, usf_ifile_list_len);
    return 0;
}

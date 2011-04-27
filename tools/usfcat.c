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

#define E_ERROR(fmt, args...)                                   \
    print_and_exit("%s:%i " fmt, __FILE__, __LINE__, args)

#define E_USF(e, name) do {                                     \
        usf_error_t _e = (e);                                   \
        if (_e != USF_ERROR_OK)                                 \
            E_ERROR("%s: %s\n", name, usf_strerror(_e));        \
    } while (0)

#define E_NULL(e, name) do {                            \
        __typeof__(e) _e = (e);                         \
        if (!_e)                                        \
            E_ERROR("%s: %s\n", name, strerror(errno)); \
    } while (0)


static char *usage_str = 
    "Usage: usfcat [OPTION]... [FILE]...\n"
    "Concatenate FILE(s) to standard output.\n\n"
    "  -h, --help\t\tdisplay this help and exit\n"
    "  -c, --compression\tSet compression algorithm\n"
    "  -d, --delta\t\tEnable delta compression\n"
    "  -f, --force\t\tForce concatenation\n";

typedef struct {
    int    ifile_list_len;
    char **ifile_list;

    usf_compression_t compression;
    int delta;
    int force;
} args_t;

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
    int c;

    static struct option long_opts[] = {
        {"help", no_argument, NULL, 'h'},
        {"compression", required_argument, NULL, 'c'},
        {"delta", no_argument, NULL, 'd'},
        {"force", no_argument, NULL, 'f'},
        { NULL, 0, NULL, 0 }
    };

    args->compression = (usf_compression_t)-1;
    args->delta = 0;
    args->force = 0;

    while ((c = getopt_long(argc, argv, "hc:df", long_opts, NULL)) != -1) {
        switch (c) {
        case 'h':
            printf("%s\n", usage_str);
            exit(EXIT_SUCCESS);

        case 'c':
            if (!strcmp("none", optarg))
                args->compression = USF_COMPRESSION_NONE;
            else if (!strcmp("bzip2", optarg))
                args->compression = USF_COMPRESSION_BZIP2;
            else
                print_and_exit("Unknown compression\n\n%s\n", usage_str);
            break;

        case 'd':
            args->delta = 1;
            break;

        case 'f':
            args->force = 1;
            break;

        case '?':
        case ':':
            print_and_exit("\n%s\n", usage_str);

        default:
            abort();
        }
    }

    if (optind < argc) {
        args->ifile_list = &argv[optind];
        args->ifile_list_len = argc - optind;
    } else
        print_and_exit("No input file specified\n"
                       "\n"
                       "%s\n", usage_str);
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

static int
check_input_files(usf_file_t    **usf_ifile_list,
                  int             usf_ifile_list_len)
{
    int ret_error = 0;
    usf_error_t error;
    const usf_header_t *base_header;

    error = usf_header(&base_header, usf_ifile_list[0]);
    E_USF(error, "usf_header");

#define FLAG_CMP(h1, h2, mask) (((h1)->flags & (mask)) == ((h2)->flags & (mask)))
    for (int i = 1; i < usf_ifile_list_len; i++) {
        const usf_header_t *header;

        error = usf_header(&header, usf_ifile_list[i]);
        E_USF(error, "usf_header");

        if (!FLAG_CMP(base_header, header, USF_FLAG_TIME_MASK)) {
            fprintf(stderr, "Error: File 0 and %i have different time bases\n",
                    i);
            ret_error = 1;
        }

        if (!FLAG_CMP(base_header, header, USF_FLAG_TRACE)) {
            fprintf(stderr,
                    "Error: Mixing trace and sample files (0, %i)\n",
                    i);
            ret_error = 1;
        }

        if (!FLAG_CMP(base_header, header, USF_FLAG_INSTRUCTIONS)) {
            fprintf(stderr,
                    "Error: Mixing instruction and data samples (0, %i)\n",
                    i);
            ret_error = 1;
        }
    }
#undef FLAG_CMP

    return !ret_error;
}

static void
make_header(usf_file_t    **usf_ifile_list,
            int             usf_ifile_list_len,
            usf_header_t   *outheader)
{
    usf_error_t error;
    const usf_header_t *main_header;

    error = usf_header(&main_header, usf_ifile_list[0]);
    E_USF(error, "usf_header");

    memset(outheader, 0, sizeof(*outheader));
    outheader->version = USF_VERSION_CURRENT;
    outheader->flags = USF_FLAG_NATIVE_ENDIAN;
    outheader->compression = main_header->compression;
    outheader->time_begin = (usf_wtime_t)-1;

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

    if (!check_input_files(usf_ifile_list, usf_ifile_list_len)) {
        if (args.force)
            fprintf(stderr, "Continuing despite errors...\n");
        else {
            fprintf(stderr, "Input file errors, aborting.\n");
            close_infiles(usf_ifile_list, usf_ifile_list_len);
            exit(EXIT_FAILURE);
        }
    }

    make_header(usf_ifile_list, usf_ifile_list_len, &header);
    if (args.compression != (usf_compression_t)-1)
        header.compression = args.compression;

    if (args.delta)
        header.flags |= USF_FLAG_DELTA;

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

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */

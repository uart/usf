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

#define E_USF(e, name) do {                                     \
    usf_error_t _e = (e);                                       \
    if (_e != USF_ERROR_OK)                                     \
        print_and_exit("%s: %s\n", name, usf_strerror(_e));     \
} while (0)

static char *usage_str = "Usage: usfstats [INFILE]";

typedef struct {
    char *file_name;
} args_t;

typedef unsigned long stats_t[USF_EVENT_TRACE + 1];
#define STATS_CLS(s)    bzero(s, sizeof(stats_t))
#define STATS_INC(s, t) s[t]++ 

static inline void
STATS_PRINT(stats_t s)
{
    printf("events:     %lu\n", s[USF_EVENT_SAMPLE] +
                                s[USF_EVENT_DANGLING] +
                                s[USF_EVENT_TRACE]);
    printf("samples:    %lu\n", s[USF_EVENT_SAMPLE]);
    printf("danglings:  %lu\n", s[USF_EVENT_DANGLING]);
    printf("traces:     %lu\n", s[USF_EVENT_TRACE]);
}

static inline void
stats_inc(stats_t s, usf_event_type_t type)
{
    s[type]++;
}

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
    if (argc != 2) 
        print_and_exit("%s\n", usage_str);

    args->file_name = argv[1];
}

int
main(int argc, char **argv)
{
    args_t args;
    usf_file_t *usf_file;
    usf_event_t event;
    usf_error_t error;
    stats_t stats_burst;
    stats_t stats_global;
    unsigned long  burst_id = 0;

    parse_args(&args, argc, argv);

    error = usf_open(&usf_file, args.file_name);
    E_USF(error, "usf_open");

    STATS_CLS(stats_burst);
    STATS_CLS(stats_global);
    while (usf_read(usf_file, &event) == USF_ERROR_OK) {
        STATS_INC(stats_burst, event.type);
        STATS_INC(stats_global, event.type);
        if (event.type == USF_EVENT_BURST) {
            printf("=== Burst stats [%lu] === \n", burst_id++);
            STATS_PRINT(stats_burst);
            STATS_CLS(stats_burst);
        }
    }

    /* Assuming that the laste event is not a burst */
    printf("=== Burst stats [%lu] === \n", burst_id++);
    STATS_PRINT(stats_burst);


    printf("\n=== Global stats ===\n");
    STATS_PRINT(stats_global);

    error = usf_close(usf_file);
    E_USF(error, "usf_close");
    return 0;
}

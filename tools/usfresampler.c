/*
 * Copyright (C) 2010-2011, David Eklov
 * Copyright (C) 2010-2011, Andreas Sandberg
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
#include <getopt.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <uart/usf.h>

static char svn_id[] = "$Id: usfresampler.c 2045 2011-02-02 14:44:45Z ansan501 $";

#define E(err, str) do {                                        \
    if ((err) != USF_ERROR_OK)                                  \
        print_and_exit("%s:%d: %s\n",                           \
                __FUNCTION__, __LINE__, usf_strerror(err));     \
} while (0)

typedef struct {
    char *i_file_name;
    char *o_file_name;
    double sample_period;
    unsigned int seed;
} args_t;

static void
print_and_exit(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    exit(EXIT_FAILURE);
}

static void
exit_usage(const char *error)
{
    fprintf(error ? stderr : stdout,
	    "Usage: usfresampler [OPTIONS] [INFILE]\n"
	    " Resamples a usf sample file.\n"
	    "      -h              Print this message.\n"
	    "      -o FILE         Output file name.\n"
	    "      -p PERIOD       Sample period.\n"
	    "      -s SEED         Random seed.\n"
	    "\n"
	    "%s\n",
	    svn_id);

    if (error) {
	fprintf(stderr,
		"\n"
		"Error: %s\n",
		error);

	exit(EXIT_FAILURE);
    } else
	exit(EXIT_SUCCESS);
}

static void
parse_args(args_t *args, int argc, char **argv)
{
    int c;

    args->i_file_name = NULL;
    args->o_file_name = NULL;
    args->sample_period = 1000;
    args->seed = time(NULL);

    while ((c = getopt(argc, argv, "ho:p:s:")) != -1) {
        switch (c) {
            case 'h':
                exit_usage(NULL);
                break;
            case 'o':
                args->o_file_name = optarg;
                break;
            case 'p':
                args->sample_period = atof(optarg);
                break;
            case 's':
                args->seed = atoi(optarg);
                break;
            default:
                assert(0);
        }
    }

    if (optind >= argc)
	exit_usage("No input file specified.");
    else if (optind + 1 != argc)
	exit_usage("Only one argument allowed.");
    else
        args->i_file_name = argv[optind];

}

static unsigned int
rnd_exp(float period)
{
    double r = (double)rand() / RAND_MAX;
    return (unsigned int)round(period * -log(1 - r));
}

int
main(int argc, char **argv)
{
    args_t args;
    usf_file_t *usf_i_file;
    usf_file_t *usf_o_file;
    usf_header_t *header;
    usf_event_t event;
    usf_error_t error;
    unsigned int event_count = 0;
    unsigned int next_sample;

    parse_args(&args, argc, argv);

    srand(args.seed);

    error = usf_open(&usf_i_file, args.i_file_name);
    E(error, "usf_open");

    error = usf_header((const usf_header_t **)&header, usf_i_file);
    E(error, "usf_header");

    error = usf_create(&usf_o_file, args.o_file_name, header);
    E(error, "usf_create");

    error = usf_read(usf_i_file, &event);
    E(error, "usf_read");

    if (event.type != USF_EVENT_BURST)
        print_and_exit("File format error\n");

    error = usf_append(usf_o_file, &event);
    E(error, "usf_append");

    next_sample = rnd_exp(args.sample_period);
    while ((error = usf_read(usf_i_file, &event)) == USF_ERROR_OK) {
        switch (event.type) {
            case USF_EVENT_SAMPLE: case USF_EVENT_DANGLING:
                break;
            case USF_EVENT_BURST: case USF_EVENT_TRACE:
                print_and_exit("File format error\n");
                break;
        }

        if (event_count == next_sample) {
            error = usf_append(usf_o_file, &event);
            E(error, "usf_append");

            next_sample += rnd_exp(args.sample_period) + 1;
        }
        event_count++;
    }

    if (error != USF_ERROR_EOF)
        E(error, "usf_read");

    usf_close(usf_i_file);
    usf_close(usf_o_file);
    return 0;
}

/*
 * Copyright (C) 2009-2011, Andreas Sandberg
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
#include <inttypes.h>

#include <argp.h>


#include <uart/usf.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
     
typedef struct {
    int verbose;
    char *file;
} conf_t;

conf_t conf = {
    .verbose = 0,
    .file = NULL
};

static void
print_access(const usf_access_t *a)
{
    printf("[tid: %" PRIu16
	   " pc: 0x%" PRIx64
	   " addr: 0x%" PRIx64
	   " time: %" PRIu64
	   " len: %" PRIu16
	   " type: %" PRIu8 " (%s)]",

	   a->tid,
	   a->pc,
	   a->addr,
	   a->time,
	   a->len,
	   a->type, usf_stratype(a->type));
}

static void
print_event(const usf_event_t *e)
{
    switch (e->type) {
    case USF_EVENT_SAMPLE:
	printf("[SAMPLE] pc1: ");
	print_access(&e->u.sample.begin);
	printf(", pc2: ");
	print_access(&e->u.sample.end);
	printf(", ls: %u\n", (1U << e->u.sample.line_size));
	break;
    case USF_EVENT_DANGLING:
	printf("[DANGLING] pc1: ");
	print_access(&e->u.dangling.begin);
	printf(", ls: %u\n", (1U << e->u.sample.line_size));
	break;
    case USF_EVENT_BURST:
	printf("[BURST] begin_time: %" PRIu64 "\n",
	       e->u.burst.begin_time);
	break;
    case USF_EVENT_TRACE:
	printf("[TRACE] pc1: ");
	print_access(&e->u.trace.access);
	printf("\n");
	break;
    default:
	abort();
    }
}

static void
print_line_sizes(usf_line_size_mask_t line_sizes)
{
    /* Mask out reserved bits */
    line_sizes &= 0xFFFF;
    for (usf_line_size_mask_t current = 1; line_sizes;
	 line_sizes >>= 1, current <<= 1) {

	if (line_sizes & 1)
	    printf("%" PRIu32 " ", current);
    }
}

static void
print_header(const usf_header_t *h)
{
    printf(
	"Header:\n"
	"\tVersion: %" PRIu16 ".%" PRIu16 "\n"
	"\tCompression: %" PRIu16 " (%s)\n"
	"\tFlags: 0x%.8" PRIx32 "\n"
	"\tSampling time: %" PRIu64 "-%" PRIu64 "\n"
	"\tLine sizes: ",

	USF_VERSION_MAJOR(h->version), USF_VERSION_MINOR(h->version),
	h->compression, usf_strcompr(h->compression),
	h->flags,
	h->time_begin, h->time_end);

    print_line_sizes(h->line_sizes);

    printf("\n\tCommand line:\n");
    for (int i = 0; i < h->argc; i++)
	printf("\t\t%s\n", h->argv[i]);
}

static int
real_main()
{
    usf_error_t error;
    usf_file_t *file;
    const usf_header_t *header;
    usf_event_t event;

    if ((error = usf_open(&file, conf.file)) != USF_ERROR_OK) {
	fprintf(stderr, "Unable to open input file: %s\n",
		usf_strerror(error));
	return EXIT_FAILURE;
    }

    if ((error = usf_header(&header, file)) != USF_ERROR_OK) {
	fprintf(stderr, "Unable to read header: %s\n",
		usf_strerror(error));
	return EXIT_FAILURE;
    }

    print_header(header);

    while ((error = usf_read(file, &event)) == USF_ERROR_OK)
	print_event(&event);

    if (error != USF_ERROR_EOF) {
	fprintf(stderr, "Failed to read event: %s\n",
		usf_strerror(error));
	return EXIT_FAILURE;
    }

    usf_close(file);

    return 0;
}


/*** argument handling ************************************************/
const char *argp_program_version =
    "usfdump " PACKAGE_VERSION;

const char *argp_program_bug_address =
    PACKAGE_BUGREPORT;

static char doc[] =
    "Dumps the contents of a USF file in human readable form.";

static char args_doc[] = "FILE";

static struct argp_option options[] = {
    {"verbose", 'v', 0, 0, "Produce verbose output" },
    { 0 }
};
     
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    conf_t *conf = (conf_t *)state->input;

    switch (key)
    {
    case 'v':
	conf->verbose = 1;
	break;

    case ARGP_KEY_ARG:
	if (state->arg_num >= 1)
	    /* Too many arguments. */
	    argp_usage(state);

	conf->file = arg;
	break;
     
    case ARGP_KEY_END:
	break;
     
    default:
	return ARGP_ERR_UNKNOWN;
    }
    return 0;
}
     
static struct argp argp = { options, parse_opt, args_doc, doc };

int
main(int argc, char **argv)
{
    /* Parse our arguments; every option seen by parse_opt will
       be reflected in arguments. */
    argp_parse (&argp, argc, argv, 0, 0, &conf);

    return real_main();
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */

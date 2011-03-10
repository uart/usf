/*
 * Copyright (C) 2009-2011, Andreas Sandberg
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
#include <inttypes.h>
#include <string.h>

#include <argp.h>


#include <uart/usf.h>
usf_error_t usf_open_hidden(usf_file_t **file,
        const char *path, usf_compression_t override);

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
     
typedef struct {
    int delta;
    usf_compression_t compression;
    usf_compression_t override;
    char *input;
    char *output;
} conf_t;

conf_t conf = {
    .delta = 0,
    .compression = -1,
    .override = -1,
    .input = NULL,
    .output = NULL
};


/*** argument handling ************************************************/
const char *argp_program_version =
    "usf2usf " PACKAGE_VERSION;

const char *argp_program_bug_address =
    PACKAGE_BUGREPORT;

static char doc[] =
    "Decodes a USF file and writes the results to a new USF file.";

static char args_doc[] = "INPUT OUTPUT";

static struct argp_option options[] = {
    {"delta", 'd', NULL, 0, "Delta compress output" },
    {"compression", 'c', "ALGORITHM", 0,
     "Set compression algorithm. Use 'help' for a list of valid algorithms." },
    {"override", 'o', "ALGORITHM", 0,
     "Override input compression (use at your own risk)"},
    { 0 }
};

static usf_compression_t
parse_compression(const char *arg)
{
    usf_compression_t compression;

    if (!strcmp("none", arg))
        compression = USF_COMPRESSION_NONE;
    else if (!strcmp("bzip2", arg))
        compression = USF_COMPRESSION_BZIP2;
    else if (!strcmp("help", arg)) {
        printf("Supported compression types:\n");
        printf("\tnone\tDisable compression\n");
        printf("\tbzip2\n");
        exit(0);
    } else {
        fprintf(stderr, "Unsupported compression '%s'.\n", arg);
        exit(EXIT_FAILURE);
    }

    return compression;
}

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    conf_t *conf = (conf_t *)state->input;

    switch (key)
    {
    case 'd':
	conf->delta = 1;
	break;
    case 'c':
        conf->compression = parse_compression(arg);
	break;
    case 'o':
        conf->override = parse_compression(arg);
        break;

    case ARGP_KEY_ARG:
	switch (state->arg_num) {
	case 0:
	    conf->input = arg;
	    break;
	case 1:
	    conf->output = arg;
	    break;
	default:
	    /* Too many arguments. */
	    argp_usage(state);
	    break;
	}
	break;
     
    case ARGP_KEY_END:
	if (state->arg_num != 2)
	    /* Too few arguments. */
	    argp_usage(state);
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
    usf_error_t error;
    usf_file_t *input;
    usf_file_t *output;
    usf_header_t *header_in;
    usf_header_t header_out;
    usf_event_t event;
    usf_compression_t in_compression = -1;

    /* Parse our arguments; every option seen by parse_opt will
       be reflected in arguments. */
    argp_parse (&argp, argc, argv, 0, 0, &conf);

    if (conf.override != (usf_compression_t)-1)
        in_compression = conf.override;

    if ((error = usf_open_hidden(&input, conf.input,
                                 in_compression)) != USF_ERROR_OK) {
	fprintf(stderr, "Unable to open input file: %s\n",
		usf_strerror(error));
	return EXIT_FAILURE;
    }

    if ((error = usf_header((const usf_header_t **)&header_in,
                             input)) != USF_ERROR_OK) {
	fprintf(stderr, "Unable to read header: %s\n",
		usf_strerror(error));
	return EXIT_FAILURE;
    }

    header_out = *header_in;

    /* Setup flags from command line arguments */
    header_out.flags &= ~(USF_FLAG_DELTA);
    header_out.flags |= conf.delta ? USF_FLAG_DELTA : 0;

    if (conf.compression != (usf_compression_t)-1) 
        header_out.compression = conf.compression;

    if ((error = usf_create(&output, conf.output, &header_out))
	!= USF_ERROR_OK) {
	fprintf(stderr, "Unable to create output file: %s\n",
		usf_strerror(error));
	return EXIT_FAILURE;
    }

    while ((error = usf_read(input, &event)) == USF_ERROR_OK) {
	if ((error = usf_append(output, &event)) != USF_ERROR_OK) {
	    fprintf(stderr, "Unable to write event: %s\n",
		    usf_strerror(error));
	    return EXIT_FAILURE;
	}
    }

    if (error != USF_ERROR_EOF) {
	fprintf(stderr, "Failed to read event: %s\n",
		usf_strerror(error));
	return EXIT_FAILURE;
    }

    usf_close(output);
    usf_close(input);
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

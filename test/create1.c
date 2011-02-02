
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <uart/usf.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define C_E(e)						\
    do {						\
        usf_error_t error = (e);			\
        if (error != USF_ERROR_OK) {			\
            fprintf(stderr,				\
		    "USF error: %s\n",			\
		    usf_strerror(error));		\
	    abort();					\
	}						\
    } while(0)

static void
append_events(usf_file_t *file)
{
    usf_event_t trace = {
	.type = USF_EVENT_TRACE,
	.u.trace = {
	    .access = {
		.pc = 0x112233445566,
		.addr = 0xDEADBEEF,
		.time = 4,
		.tid = 0,
		.len = 8,
		.type = USF_ATYPE_RW
	    }
	}
    };

    for (int i = 0; i < 10; i++) {
	C_E(usf_append(file, &trace));
	trace.u.trace.access.pc++;
	trace.u.trace.access.addr++;
    }
}

int
main(int argc, char **argv)
{
    usf_file_t *file;

    usf_header_t header = {
	USF_VERSION_CURRENT,
	USF_COMPRESSION_NONE,

	USF_FLAG_NATIVE_ENDIAN | USF_FLAG_TRACE,

	0,
	0,

	0,

	0,
	NULL
    };

    if (argc != 2) {
	fprintf(stderr, "%s FILE\n", argv[0]);
	exit(EXIT_FAILURE);
    }

    C_E(usf_create(&file, argv[1], &header));

    append_events(file);

    C_E(usf_close(file));
}

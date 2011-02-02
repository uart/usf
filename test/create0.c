
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
    usf_event_t burst = {
	.type = USF_EVENT_BURST,
	.u.burst = {
	    .begin_time = 0
	}
    };
    usf_event_t dangling = {
	.type = USF_EVENT_DANGLING,
	.u.dangling = {
	    .begin = {
		.pc = 0x123456789ABCDEF0,
		.addr = 0xBADF00D,
		.time = 1,
		.tid = 0xFF00,
		.len = 8,
		.type = USF_ATYPE_WR
	    },
	    .line_size = 6
	}
    };
    usf_event_t sample = {
	.type = USF_EVENT_SAMPLE,
	.u.sample = {
	    .begin = {
		.pc = 0xABCDEF0123456789,
		.addr = 0xCAFE,
		.time = 3,
		.tid = 0x00FF,
		.len = 8,
		.type = USF_ATYPE_RD
	    },
	    .end = {
		.pc = 0x112233445566,
		.addr = 0xDEADBEEF,
		.time = 4,
		.tid = 0x00FF,
		.len = 8,
		.type = USF_ATYPE_RW
	    },
	    .line_size = 6
	}
    };

    usf_event_t trace = {
	.type = USF_EVENT_TRACE,
	.u.trace = {
	    .access = {
		.pc = 0x112233445566,
		.addr = 0xDEADBEEF,
		.time = 4,
		.len = 8,
		.type = USF_ATYPE_RW
	    }
	}
    };

    C_E(usf_append(file, &burst));
    C_E(usf_append(file, &dangling));
    C_E(usf_append(file, &sample));
    C_E(usf_append(file, &trace));
}

int
main(int argc, char **argv)
{
    usf_file_t *file;
    char *usf_argv[] = {
	"foo",
	"bar"
    };
    usf_header_t header = {
	USF_VERSION_CURRENT,
	USF_COMPRESSION_NONE,

	USF_FLAG_NATIVE_ENDIAN,

	123,
	456,

	0x40,

	sizeof(usf_argv) / sizeof(*usf_argv),
	usf_argv
    };

    if (argc != 2) {
	fprintf(stderr, "%s FILE\n", argv[0]);
	exit(EXIT_FAILURE);
    }

    C_E(usf_create(&file, argv[1], &header));

    append_events(file);

    C_E(usf_close(file));
}

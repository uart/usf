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

/* Define _GNU_SOURCE to expose strndup(3) */
#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stddef.h> /* offsetof */
#include <assert.h>

#include "os_compat.h"

#include "usf_header.h"
#include "error.h"

usf_error_t
usf_header_read(usf_header_t **header, FILE *f)
{
    usf_error_t error = USF_ERROR_OK;
    uint32_t header_len;
    char *c_header = NULL;
    usf_header_t *h = NULL;
    char *src_arg;
    size_t data_left;
    int i;

    E_IF_IO(f, fread(&header_len, sizeof(header_len), 1, f) != 1);
    E_IF(header_len < offsetof(usf_header_t, argv), USF_ERROR_FILE);
    E_NULL(c_header = malloc(header_len), USF_ERROR_MEM);
    
    E_IF_IO(f, fread(c_header, header_len, 1, f) != 1);
    E_NULL(h = malloc(sizeof(usf_header_t)), USF_ERROR_MEM);
    memcpy(h, c_header, offsetof(usf_header_t, argv));

    /* Set argv to NULL to prevent cleanup breakage */
    h->argv = NULL;

    E_NULL(h->argv = malloc(sizeof(char*) * h->argc), USF_ERROR_MEM);
    for (i = 0; i < h->argc; i++)
	h->argv[i] = NULL;

    src_arg = c_header + offsetof(usf_header_t, argv);
    data_left = header_len - offsetof(usf_header_t, argv);
    for (i = 0; i < h->argc; i++) {
        size_t arg_len;
        E_IF(data_left < 1, USF_ERROR_FILE);
	E_NULL(h->argv[i] = strndup(src_arg, data_left),
	       USF_ERROR_MEM);
	arg_len = strlen(h->argv[i]);
	data_left -= arg_len + 1;
	src_arg += arg_len + 1;
    }

    /* XXX: Remove assert after testing */
    assert(data_left == 0);

    *header = h;
    free(c_header);
    return USF_ERROR_OK;

ret_err:
    if (c_header)
	free(c_header);

    if (h)
	usf_header_free(h);

    return error;
}

usf_error_t
usf_header_write(FILE *f, const usf_header_t *h)
{
    usf_error_t error = USF_ERROR_OK;
    uint32_t header_len = offsetof(usf_header_t, argv);
    int i;

    for (i = 0; i < h->argc; i++)
	header_len += strlen(h->argv[i]) + 1;

    E_IF_IO(f, fwrite(&header_len, sizeof(header_len), 1, f) != 1);
    E_IF_IO(f, fwrite(h, offsetof(usf_header_t, argv), 1, f) != 1);

    for (i = 0; i < h->argc; i++)
	E_IF_IO(f, fwrite(h->argv[i], strlen(h->argv[i]) + 1, 1, f) != 1);

ret_err:
    return error;
}

usf_error_t
usf_header_free(usf_header_t *header)
{
    int i;
    if (!header)
	return USF_ERROR_PARAM;

    if (header->argv) {
	for (i = 0; i < header->argc; i++) {
	    if (header->argv[i])
		free(header->argv[i]);
	}

	free(header->argv);
    }

    free(header);

    return USF_ERROR_OK;
}

usf_error_t
usf_header_dup(usf_header_t **out, const usf_header_t *in)
{
    usf_error_t error;
    usf_header_t *h = NULL;
    int i;

    E_IF(!out || !in, USF_ERROR_PARAM);

    E_NULL(h = malloc(sizeof(usf_header_t)), USF_ERROR_MEM);

    *h = *in;

    /* Make sure pointers are set to NULL in case we need to free the
     * header due to an error. */
    h->argv = NULL;

    E_NULL(h->argv = malloc(sizeof(char*) * h->argc), USF_ERROR_MEM);

    /* Set all argument string pointer to NULL to prevent erroneous
     * frees */
    for (i = 0; i < h->argc; i++)
	h->argv[i] = NULL;

    for (i = 0; i < h->argc; i++)
	E_NULL(h->argv[i] = strdup(in->argv[i]), USF_ERROR_MEM);

    *out = h;
    return USF_ERROR_OK;

ret_err:
    if (h)
	usf_header_free(h);

    return error;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */

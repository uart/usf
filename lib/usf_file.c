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
#include <string.h>

#include "usf_priv.h"
#include "usf_header.h"
#include "usf_internal.h"
#include "error.h"

static const char usf_magic[] = "USF1";

static usf_io_methods_t io_methods[] = {
#define _COMP(comp, init, fini, read, write) [comp] = {init, fini, read, write},
    USF_COMP_LIST
#undef _COMP
};

static inline usf_error_t
check_compression(usf_compression_t comp)
{
#define _COMP(comp, u1, u2, u3, u4) case comp:
    switch (comp) { 
            USF_COMP_LIST
            return USF_ERROR_OK;
        default:
            return USF_ERROR_FILE;
    }
#undef _COMP
}


usf_error_t
read_magic(FILE *f)
{
    char magic[sizeof(usf_magic)];

    if (fread(magic, sizeof(magic), 1, f) != 1)
	return feof(f) ? USF_ERROR_FILE : USF_ERROR_SYS;

    return memcmp(magic, usf_magic, sizeof(usf_magic)) == 0 ?
	USF_ERROR_OK : USF_ERROR_FILE;
}

usf_error_t
write_magic(FILE *f)
{
    return fwrite(usf_magic, sizeof(usf_magic), 1, f) == 1 ?
	USF_ERROR_OK : USF_ERROR_SYS;
}

/* This function is not exported, i.e. it prototype is not in usf.h, it is
 * only meant to be called by usf2usf */
usf_error_t
usf_open_hidden(usf_file_t **file, const char *path,
                usf_compression_t override)
{
    usf_file_t *f = NULL;
    usf_error_t error;

    E_IF(!file, USF_ERROR_PARAM);

    f = malloc(sizeof(usf_file_t));
    E_NULL(f, USF_ERROR_MEM);

    f->header = NULL;
    f->bzeof = 0;

    if (path)
        f->file = fopen(path, "r");
    else
        f->file = stdin;

    E_NULL(f->file, USF_ERROR_SYS);
    memset(&f->last_access, 0, sizeof(f->last_access));

    E_ERROR(read_magic(f->file));
    E_ERROR(usf_header_read(&f->header, f->file));

    if (override != (usf_compression_t)-1)
        f->header->compression = override;

    E_ERROR(check_compression(f->header->compression));
    f->io_methods = &io_methods[f->header->compression];
    E_ERROR(usf_internal_init(f, USF_MODE_READ));

    *file = f;
    return USF_ERROR_OK;

ret_err:
    if (f) {
	if (f->file)
	    fclose(f->file);
	if (f->header)
	    usf_header_free(f->header);
	free(f);
    }

    return error;
}

usf_error_t
usf_open(usf_file_t **file, const char *path)
{
    return usf_open_hidden(file, path, -1);
}

usf_error_t
usf_create(usf_file_t **file,
	   const char *path, const usf_header_t *header)
{
    usf_file_t *f;
    usf_error_t error;

    E_IF(!file || !header, USF_ERROR_PARAM);
    /* Currently we require the native endian bit to be set. We could
     * support having the foreign bit set instead, but we won't do
     * that at the moment. Having both bits set will always constitue
     * an error. */
    E_IF(!(header->flags & USF_FLAG_NATIVE_ENDIAN) ||
    header->flags & USF_FLAG_FOREIGN_ENDIAN, USF_ERROR_PARAM);

    f = malloc(sizeof(usf_file_t));
    E_NULL(f, USF_ERROR_MEM);

    f->header = NULL;

    if (path)
        f->file = fopen(path, "w");
    else
        f->file = stdout;

    E_NULL(f->file, USF_ERROR_SYS);
    memset(&f->last_access, 0, sizeof(f->last_access));

    E_ERROR(write_magic(f->file));
    E_ERROR(usf_header_dup(&f->header, header));
    E_ERROR(usf_header_write(f->file, f->header));
    
    E_ERROR(check_compression(f->header->compression));
    f->io_methods = &io_methods[f->header->compression];
    E_ERROR(usf_internal_init(f, USF_MODE_WRITE));

    *file = f;
    return USF_ERROR_OK;

ret_err:
    if (f) {
	if (f->file)
	    fclose(f->file);
	if (f->header)
	    usf_header_free(f->header);
	free(f);
    }

    return error;
}

usf_error_t
usf_close(usf_file_t *file)
{
    if (!file || !file->file)
	return USF_ERROR_PARAM;

    usf_internal_fini(file);
    usf_header_free(file->header);
    fclose(file->file);
    free(file);
    return USF_ERROR_OK;
}

usf_error_t
usf_header(const usf_header_t **header, usf_file_t *file)
{
    if (!file)
	return USF_ERROR_PARAM;

    *header = file->header;
    return USF_ERROR_OK;
}

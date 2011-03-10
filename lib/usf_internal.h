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

#ifndef USF_INTERNAL_H
#define USF_INTERNAL_H

#include <assert.h>
#include "usf_priv.h"
#include "error.h"

enum {
    USF_MODE_READ,
    USF_MODE_WRITE,
};

typedef struct usf_io_methods_s {
    usf_error_t (*init)(usf_file_t *file, int mode);
    usf_error_t (*fini)(usf_file_t *file);
    usf_error_t (*read)(usf_file_t *file, void *buf, size_t count);
    usf_error_t (*write)(usf_file_t *file, void *buf, size_t count);
} usf_io_methods_t;

#define USF_COMP_LIST                                                          \
 _COMP(USF_COMPRESSION_NONE,  init_none,  fini_none,  read_none,  write_none)  \
 _COMP(USF_COMPRESSION_BZIP2, init_bzip2, fini_bzip2, read_bzip2, write_bzip2) \


usf_error_t init_none(usf_file_t *file, int mode);
usf_error_t fini_none(usf_file_t *file);
usf_error_t read_none(usf_file_t *file, void *buf, size_t count);
usf_error_t write_none(usf_file_t *file, void *buf, size_t count);

usf_error_t init_bzip2(usf_file_t *file, int mode);
usf_error_t fini_bzip2(usf_file_t *file);
usf_error_t read_bzip2(usf_file_t *file, void *buf, size_t count);
usf_error_t write_bzip2(usf_file_t *file, void *buf, size_t count);


static inline usf_error_t
usf_internal_init(usf_file_t *file, int mode)
{
    assert(file && file->io_methods && file->io_methods->init);
    file->mode = mode;
    return file->io_methods->init(file, mode);
}

static inline usf_error_t
usf_internal_fini(usf_file_t *file)
{
    assert(file && file->io_methods && file->io_methods->fini);
    return file->io_methods->fini(file);
}

static inline usf_error_t
usf_internal_read(usf_file_t *file, void *buf, size_t count)
{
    assert(file && file->io_methods && file->io_methods->read);
    return file->io_methods->read(file, buf, count);
}

static inline usf_error_t
usf_internal_write(usf_file_t *file, void *buf, size_t count)
{
    assert(file && file->io_methods && file->io_methods->write);
    return file->io_methods->write(file, buf, count);
}

#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */

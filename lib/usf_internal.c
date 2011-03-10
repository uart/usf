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

#include <unistd.h>
#include <assert.h>
#include <bzlib.h>

#include "usf_priv.h"
#include "usf_internal.h"


/* ********************************************************************** */

usf_error_t
init_none(usf_file_t *file, int mode)
{
    return USF_ERROR_OK;
}

usf_error_t
fini_none(usf_file_t *file)
{
    return USF_ERROR_OK;
}

usf_error_t
read_none(usf_file_t *file, void *buf, size_t count)
{
    usf_error_t error = USF_ERROR_OK;

    if (fread(buf, count, 1, file->file) != 1)
        error = feof(file->file) ? USF_ERROR_EOF : USF_ERROR_SYS;

    return error;
}

usf_error_t
write_none(usf_file_t *file, void *buf, size_t count)
{
    usf_error_t error = USF_ERROR_OK;

    if (fwrite(buf, count, 1, file->file) != 1)
        error = USF_ERROR_SYS;

    return error;
}

/* ********************************************************************** */

usf_error_t
init_bzip2(usf_file_t *file, int mode)
{
    int bzerror;

    if (mode == USF_MODE_READ) 
        file->bzfile = BZ2_bzReadOpen(&bzerror, file->file, 0, 0, NULL, 0);
    else
        file->bzfile = BZ2_bzWriteOpen(&bzerror, file->file, 1, 0, 30);

    if (bzerror != BZ_OK)
        return USF_ERROR_SYS;

    return USF_ERROR_OK;
}

usf_error_t
fini_bzip2(usf_file_t *file)
{
    int bzerror;

    if (file->mode == USF_MODE_READ)
        BZ2_bzReadClose(&bzerror, file->bzfile);
    else
        BZ2_bzWriteClose(&bzerror, file->bzfile, 0, NULL, NULL);

    if (bzerror != BZ_OK)
        return USF_ERROR_SYS;

    return USF_ERROR_OK;
}

usf_error_t
read_bzip2(usf_file_t *file, void *buf, size_t count)
{
    int bzerror;
    int read;
    usf_error_t usf_error = USF_ERROR_OK;

     /* Error handling: If BZ2_bzRead(bzerror, b, count) reads the last count
      * bytes in the file, it returns count and sets bzerror to BZ_STREAM_END,
      * in this case we want to return USF_ERROR_OK, since there are no errors,
      * and then return USF_ERROR_EOF on the next call.
      *
      * Note: If BZ2_bzRead is called once more after it has returned
      * BZ_STREAM_END it seems to always return BZ_SEQUENCE_ERROR, which
      * we could check for. However, according to the manual
      * BZ_SEQUENCE_ERROR is returned "if b was opened with BZ2_bzWriteOpen".
      *
      * --  David E.
      */

    if (file->bzeof)
        return USF_ERROR_EOF;

    read = BZ2_bzRead(&bzerror, file->bzfile, buf, count);
    if (bzerror != BZ_OK) {
        if (bzerror == BZ_STREAM_END) {
            file->bzeof = 1;
            if (read != count)
                usf_error = USF_ERROR_FILE;
        } else
            usf_error = USF_ERROR_SYS;
    }

    return usf_error;
}

usf_error_t
write_bzip2(usf_file_t *file, void *buf, size_t count)
{
    int bzerror;

    BZ2_bzWrite(&bzerror, file->bzfile, buf, count);
    return bzerror != BZ_OK ? USF_ERROR_SYS : USF_ERROR_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */

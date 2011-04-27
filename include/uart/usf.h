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

#ifndef UART_USF_H
#define UART_USF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <uart/usf_types.h>
#include <uart/usf_events.h>

#define USF_VERSION(maj, min) (((maj & 0xFFFF) << 16) | ((min) & 0xFFFF))
#define USF_VERSION_MAJOR(v) (((v) >> 16) & 0xFFFF)
#define USF_VERSION_MINOR(v) ((v) & 0xFFFF)
    
#define USF_VERSION_CURRENT_MAJOR 0
#define USF_VERSION_CURRENT_MINOR 2
#define USF_VERSION_CURRENT (USF_VERSION(USF_VERSION_CURRENT_MAJOR,     \
					 USF_VERSION_CURRENT_MINOR))

typedef uint16_t usf_version_t;

enum {
    USF_COMPRESSION_NONE = 0,
    USF_COMPRESSION_BZIP2
};

/** Compression type */
typedef uint16_t usf_compression_t;

/** The file contains a trace if this flag is set, samples otherwise. */
#define USF_FLAG_TRACE (1 << 0)

/** The file was burst sampled, expect burst events. */
#define USF_FLAG_BURST (1 << 1)

/** File uses delta compression of accesses */
#define USF_FLAG_DELTA (1 << 2)

/** File contains instruction samples */
#define USF_FLAG_INSTRUCTIONS (1 << 3)

/**
 * Reserve two bits for the time base, note that these may not be
 * contiguous in future releases.
 */
#define USF_FLAG_TIME_MASK ((1 << 4) | (1 << 5))

/** Time in accesses */
#define USF_FLAG_TIME_ACCESSES (0 << 4)
/** Time in instructions */
#define USF_FLAG_TIME_INSTRUCTIONS (1 << 4)
/** Time in cycles */
#define USF_FLAG_TIME_CYCLES (3 << 4)

/* @{ */
/**
 * Always set the native endian flag when creating a file. If the
 * native endian flag is set when reading the file, the file was
 * created by a system with the same endianness as the current
 * system. The foreign endian flag is only set (due to the difference
 * endianness) if the file was created on a system with another
 * endianness.
 *
 * The native and foreign endian flags are, for obvious reasons,
 * mutualy exclusive and one MUST be set.
 */
#define USF_FLAG_NATIVE_ENDIAN (1 << 7)
#define USF_FLAG_FOREIGN_ENDIAN (1 << 31)
/* @} */

/** File format flags */
typedef uint32_t usf_flags_t;

/** Mask of line sizes. Bit value is line size. Bit 16-31 are reserved
 * and must be 0. */
typedef uint32_t usf_line_size_mask_t;

typedef struct {
    usf_version_t version;
    usf_compression_t compression;

    usf_flags_t flags;

    usf_wtime_t time_begin;
    usf_wtime_t time_end;

    usf_line_size_mask_t line_sizes;

    uint32_t argc;
    char **argv;
} usf_header_t;

/** Errors returned by the library */
typedef enum {
    /** No error */
    USF_ERROR_OK = 0,
    /** Invalid parameter */
    USF_ERROR_PARAM,
    /** System error */
    USF_ERROR_SYS,
    /** Memory allocation failure */
    USF_ERROR_MEM,
    /** End of file */
    USF_ERROR_EOF,
    /** File format error */
    USF_ERROR_FILE,
    /** Unsupported feature */
    USF_ERROR_UNSUPPORTED
} usf_error_t;

typedef struct usf_file_s usf_file_t;

/**
 * Return a string representation of an error code. The returned
 * pointer is owned by the library.
 */
const char *usf_strerror(usf_error_t error);

/**
 * Return a string representation of a compression type. The returned
 * pointer is owned by the library.
 */
const char *usf_strcompr(usf_compression_t compression);

/**
 * Return a string representation of an access type. The returned
 * pointer is owned by the library.
 */
const char *usf_stratype(usf_atype_t type);

/**
 * Open a file for reading. The returned file pointer is undefined if
 * the procedure returns an error.
 *
 * \param file Returned file object.
 * \param path Path to file.
 * \return USF_ERROR_OK on success.
 */
usf_error_t usf_open(usf_file_t **file, const char *path);

/**
 * Creates a new file, overwriting any existing file with the same
 * name. The returned file pointer is undefined if the procedure
 * returns an error.
 *
 * \param file Returned file object.
 * \param path Path to the new file
 * \param header Header for the file
 * \return USF_ERROR_OK on success.
 */
usf_error_t usf_create(usf_file_t **file,
		       const char *path, const usf_header_t *header);

/**
 * Close a file and deallocate all resources associated with the file.
 *
 * \param file Pointer to open file object.
 * \return USF_ERROR_OK on success.
 */
usf_error_t usf_close(usf_file_t *file);

/**
 * Retrieves the header of an existing file. The contents of header
 * pointer are undefined if the procedure returns an error.
 *
 * \param header Pointer to the target header pointer.
 * \param file Pointer to an open file object.
 * \return USF_ERROR_OK on success.
 */
usf_error_t usf_header(const usf_header_t **header, usf_file_t *file);

/**
 * Append an event to a file that has been opened for writing.
 *
 * \param file File object opened for writing.
 * \param event Event to append to the file.
 * \return USF_ERROR_OK on success.
 */
usf_error_t usf_append(usf_file_t *file, const usf_event_t *event);
/**
 * Read the next event in the file. The contents of event are
 * undefined if the procedure fails.
 *
 * \param file Pointer a file.
 * \param event Pointer to an event structure.
 * \return USF_ERROR_OK on success, USF_ERROR_EOF on end of file,
 *         USF_ERROR_FILE on file format errors.
 */
usf_error_t usf_read(usf_file_t *file, usf_event_t *event);

#ifdef __cplusplus
}
#endif

#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */

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

#ifndef UART_USF_TYPES_H
#define UART_USF_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** Address in virtual memory */
typedef uint64_t usf_addr_t;
/** Access time in number of memory accesses since some arbitrary point */
typedef uint64_t usf_atime_t;
/** Time in seconds since the Epoch (UTC) */
typedef uint64_t usf_wtime_t;
/** Access length (bytes) */
typedef uint16_t usf_alen_t;
/** Thread ID */
typedef uint16_t usf_tid_t;
/** Log2 line size */
typedef uint8_t usf_line_size_2_t;

#define USF_ALEN_UNKNOWN (usf_alen_t) -1
#define USF_TID_UNKNOWN (usf_tid_t) -1

/** Access types */
enum {
    /** Read */
    USF_ATYPE_RD = 0,
    /** Write */
    USF_ATYPE_WR,
    /** Read & Write */
    USF_ATYPE_RW,

    /** Intel PREFETCHNTA */
    USF_ATYPE_PREFETCHNTA,
    /** Intel PREFETCHT0 */
    USF_ATYPE_PREFETCHT0,
    /** Intel PREFETCHT2 */
    USF_ATYPE_PREFETCHT1,
    /** Intel PREFETCHT2 */
    USF_ATYPE_PREFETCHT2,

    /** AMD PREFETCH */
    USF_ATYPE_PREFETCH,
    /** AMD PREFETCHW */
    USF_ATYPE_PREFETCHW,

    /** Not available */
    USF_ATYPE_UNKNOWN,

    /** Instruction sample */
    USF_ATYPE_INSTRUCTION,
};
typedef uint8_t usf_atype_t;

/** Memory access */
typedef struct {
    usf_addr_t pc;
    usf_addr_t addr;
    usf_atime_t time;

    usf_tid_t tid;
    usf_alen_t len;
    usf_atype_t type;
} usf_access_t;

#ifdef __cplusplus
}
#endif

#endif

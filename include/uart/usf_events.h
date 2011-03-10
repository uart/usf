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

#ifndef UART_USF_EVENTS_H
#define UART_USF_EVENTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <uart/usf_types.h>

enum {
    USF_EVENT_SAMPLE = 0,
    USF_EVENT_DANGLING,
    USF_EVENT_BURST,
    USF_EVENT_TRACE
};

typedef uint8_t usf_event_type_t;

/** Sample between two accesses */
typedef struct {
    usf_access_t begin;
    usf_access_t end;
    usf_line_size_2_t line_size;
} usf_event_sample_t;

/** Dangling sample */
typedef struct {
    usf_access_t begin;
    usf_line_size_2_t line_size;
} usf_event_dangling_t;

/** Dangling sample */
typedef struct {
    usf_access_t access;
} usf_event_trace_t;

/**
 * Beginning of a burst in pc1 time.
 *
 * Samples belonging to a burst can only occur after the burst
 * marker. A burst continues until the time of the next burst
 * marker. Note that samples needn't belong to the current burst, but
 * may belong to any previous burst depending on their start time.
 */
typedef struct {
    usf_atime_t begin_time;
} usf_event_burst_t;

typedef struct {
    usf_event_type_t type;

    union {
	usf_event_sample_t sample;
	usf_event_dangling_t dangling;
	usf_event_burst_t burst;
	usf_event_trace_t trace;
    } u;
} usf_event_t;

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

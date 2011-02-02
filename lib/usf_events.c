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

#include <stdio.h>
#include <assert.h>

#include "usf_priv.h"
#include "usf_internal.h"
#include "error.h"

typedef struct {
    usf_error_t (*write)(usf_file_t *file, const usf_event_t *event);
    usf_error_t (*read)(usf_file_t *file, usf_event_t *event);
} event_io_t;


/* ********************************************************************** */

#define DATA_LEN_ACCESS (2*sizeof(usf_addr_t) + \
			 sizeof(usf_atime_t) + \
			 sizeof(usf_tid_t) + \
			 sizeof(usf_alen_t) + \
			 sizeof(usf_atype_t))

#define D_DELTA_pc (1 << 0)
#define D_DELTA_addr (1 << 1)
#define D_DELTA_time (1 << 2)

#define D_CONST_tid (1 << 4)
#define D_CONST_len (1 << 5)
#define D_CONST_type (1 << 6)

#define PACK_UINT64(file, flags, buf, a, field)	\
    pack_uint64(flags, buf, \
		&file->last_access.field, a->field, D_DELTA_ ## field)

#define PACK_UINT16(file, flags, buf, a, field)	\
    pack_uint16(flags, buf, \
		&file->last_access.field, a->field, D_CONST_ ## field)

#define PACK_UINT8(file, flags, buf, a, field)	\
    pack_uint8(flags, buf, \
		&file->last_access.field, a->field, D_CONST_ ## field)


#define UNPACK_UINT64(file, flags, buf, a, field)			\
    (a->field = unpack_uint64(flags, buf,				\
			      &file->last_access.field, D_DELTA_ ## field))

#define UNPACK_UINT16(file, flags, buf, a, field)			\
    (a->field = unpack_uint16(flags, buf,				\
			      &file->last_access.field, D_CONST_ ## field))

#define UNPACK_UINT8(file, flags, buf, a, field)			\
    (a->field = unpack_uint8(flags, buf,				\
			     &file->last_access.field, D_CONST_ ## field))

static inline size_t
delta_data_size(uint8_t flags)
{
    size_t s = 0;

    s += flags & D_DELTA_pc ? 1 : 8;
    s += flags & D_DELTA_addr ? 1 : 8;
    s += flags & D_DELTA_time ? 1 : 8;

    s += flags & D_CONST_tid ? 0 : 2;
    s += flags & D_CONST_len ? 0 : 2;
    s += flags & D_CONST_type ? 0 : 1;

    return s;
}

static inline void
pack_uint64(char *flags, char **buf,
	    uint64_t *ref, uint64_t val, uint8_t flag)
{
    int8_t delta = (int8_t)(val - *ref);

    if (delta + *ref == val) {
	*flags |= flag;
	*((int8_t *)*buf) = delta;
	*buf += 1;
    } else {
	*((uint64_t *)*buf) = val;
	*buf += 8;
    }

    *ref = val;
}

static inline uint64_t
unpack_uint64(char flags, char **buf, uint64_t *ref, uint8_t flag)
{
    if (flags & flag) {
	*ref += *((int8_t*)*buf);
	*buf += 1;
    } else {
	*ref = *((uint64_t*)*buf);
	*buf += 8;
    }
    return *ref;
}


static inline void
pack_uint16(char *flags, char **buf,
	    uint16_t *ref, uint16_t val, uint8_t flag)
{
    if (val == *ref)
	*flags |= flag;
    else {
	*((uint16_t *)*buf) = val;
	*buf += 2;
	*ref = val;
    }
}

static inline uint16_t
unpack_uint16(char flags, char **buf, uint16_t *ref, uint8_t flag)
{
    if (!(flags & flag)) {
	*ref = *((uint16_t*)*buf);
	*buf += 2;
    }
    return *ref;
}

static inline void
pack_uint8(char *flags, char **buf,
	   uint8_t *ref, uint8_t val, uint8_t flag)
{
    if (val == *ref) {
	*flags |= flag;
    } else {
	*((uint8_t *)*buf) = val;
	*buf += 1;
	*ref = val;
    }
}

static inline uint8_t
unpack_uint8(char flags, char **buf, uint8_t *ref, uint8_t flag)
{
    if (!(flags & flag)) {
	*ref = *((uint8_t*)*buf);
	*buf += 1;
    }
    return *ref;
}

static usf_error_t
write_access(usf_file_t *file, const usf_access_t *a)
{
    usf_error_t error = USF_ERROR_OK;

    if (file->header->flags & USF_FLAG_DELTA) {
	char buf[DATA_LEN_ACCESS + 1];
	char *cur = buf + 1;
	*buf = 0;
	PACK_UINT64(file, buf, &cur, a, pc);
	PACK_UINT64(file, buf, &cur, a, addr);
	PACK_UINT64(file, buf, &cur, a, time);

	PACK_UINT16(file, buf, &cur, a, tid);
	PACK_UINT16(file, buf, &cur, a, len);
	PACK_UINT8(file, buf, &cur, a, type);
        
        E_ERROR(usf_internal_write(file, (void *)buf, cur - buf));
    } else
        E_ERROR(usf_internal_write(file, (void *)a, DATA_LEN_ACCESS));

ret_err:
    return error;
}

static usf_error_t
read_access(usf_file_t *file, usf_access_t *a)
{
    usf_error_t error = USF_ERROR_OK;

    if (file->header->flags & USF_FLAG_DELTA) {
	char buf[DATA_LEN_ACCESS + 1];
	char *cur = buf + 1;
	size_t size;

	E_ERROR(usf_internal_read(file, (void *)buf, 1));

	size = delta_data_size(*buf);
	E_ERROR(usf_internal_read(file, (void *)cur, size));

	UNPACK_UINT64(file, *buf, &cur, a, pc);
	UNPACK_UINT64(file, *buf, &cur, a, addr);
	UNPACK_UINT64(file, *buf, &cur, a, time);

	UNPACK_UINT16(file, *buf, &cur, a, tid);
	UNPACK_UINT16(file, *buf, &cur, a, len);
	UNPACK_UINT8(file, *buf, &cur, a, type);
    } else
        E_ERROR(usf_internal_read(file, (void *)a, DATA_LEN_ACCESS));

ret_err:
    return error;
}

/* ********************************************************************** */

static usf_error_t
write_sample(usf_file_t *file, const usf_event_t *event)
{
    usf_error_t error = USF_ERROR_OK;
    const usf_event_sample_t *s = &event->u.sample;
    assert(event->type == USF_EVENT_SAMPLE);

    E_ERROR(write_access(file, &s->begin));
    E_ERROR(write_access(file, &s->end));
    E_ERROR(usf_internal_write(file, (void *)&s->line_size,
                               sizeof(usf_line_size_2_t)));

ret_err:
    return error;
}

static usf_error_t
read_sample(usf_file_t *file, usf_event_t *event)
{
    usf_error_t error = USF_ERROR_OK;
    usf_event_sample_t *s = &event->u.sample;
    assert(event->type == USF_EVENT_SAMPLE);

    E_ERROR(read_access(file, &s->begin));
    E_ERROR(read_access(file, &s->end));
    E_ERROR(usf_internal_read(file, (void *)&s->line_size,
                              sizeof(usf_line_size_2_t)));

ret_err:
    return error;
}

/* ********************************************************************** */

static usf_error_t
write_dangling(usf_file_t *file, const usf_event_t *event)
{
    usf_error_t error = USF_ERROR_OK;
    const usf_event_dangling_t *d = &event->u.dangling;
    assert(event->type == USF_EVENT_DANGLING);

    E_ERROR(write_access(file, &d->begin));
    E_ERROR(usf_internal_write(file, (void *)&d->line_size,
                               sizeof(usf_line_size_2_t)));

ret_err:
    return error;
}

static usf_error_t
read_dangling(usf_file_t *file, usf_event_t *event)
{
    usf_error_t error = USF_ERROR_OK;
    usf_event_dangling_t *d = &event->u.dangling;
    assert(event->type == USF_EVENT_DANGLING);

    E_ERROR(read_access(file, &d->begin));
    E_ERROR(usf_internal_read(file, (void *)&d->line_size,
                              sizeof(usf_line_size_2_t)));

ret_err:
    return error;
}

/* ********************************************************************** */

static usf_error_t
write_burst(usf_file_t *file, const usf_event_t *event)
{
    usf_error_t error = USF_ERROR_OK;
    const usf_event_burst_t *b = &event->u.burst;
    assert(event->type == USF_EVENT_BURST);

    E_ERROR(usf_internal_write(file, (void *)&b->begin_time,
                               sizeof(usf_atime_t)));

ret_err:
    return error;
}

static usf_error_t
read_burst(usf_file_t *file, usf_event_t *event)
{
    usf_error_t error = USF_ERROR_OK;
    usf_event_burst_t *b = &event->u.burst;
    assert(event->type == USF_EVENT_BURST);

    E_ERROR(usf_internal_read(file, (void *)&b->begin_time, 
                              sizeof(usf_atime_t)));

ret_err:
    return error;
}


/* ********************************************************************** */

/* The following handles writes a trace _event_, pure trace files
 * (i.e. files having USF_FLAG_TRACE set) use different trace event
 * encodings.
 */

static usf_error_t
read_trace(usf_file_t *file, usf_event_t *event)
{
    usf_error_t error = USF_ERROR_OK;
    usf_event_trace_t *d = &event->u.trace;
    assert(event->type == USF_EVENT_TRACE);

    E_ERROR(read_access(file, &d->access));

ret_err:
    return error;
}

static usf_error_t
write_trace(usf_file_t *file, const usf_event_t *event)
{
    usf_error_t error = USF_ERROR_OK;
    const usf_event_trace_t *d = &event->u.trace;
    assert(event->type == USF_EVENT_TRACE);

    E_ERROR(write_access(file, &d->access));

ret_err:
    return error;
}

/* ********************************************************************** */

event_io_t event_io[] = {
    { &write_sample, &read_sample },
    { &write_dangling, &read_dangling },
    { &write_burst, &read_burst },
    { &write_trace, &read_trace }
};

static usf_error_t
usf_append_event(usf_file_t *file, const usf_event_t *event)
{
    usf_error_t error = USF_ERROR_OK;

    E_ERROR(usf_internal_write(file, (void *)&event->type,
                               sizeof(usf_event_type_t)));
    E_ERROR(event_io[event->type].write(file, event));

ret_err:
    return error;
}


static usf_error_t
usf_append_trace(usf_file_t *file, const usf_event_t *event)
{
    usf_error_t error = USF_ERROR_OK;

    assert(event->type == USF_EVENT_TRACE);
    E_ERROR(write_access(file, &event->u.trace.access));

ret_err:
    return error;
}

usf_error_t
usf_append(usf_file_t *file, const usf_event_t *event)
{
    if(!file || !event || (event->type >= ARRAY_LEN(event_io)))
       return USF_ERROR_PARAM;

    if (file->header->flags & USF_FLAG_TRACE)
	return usf_append_trace(file, event);
    else
	return usf_append_event(file, event);
}

static usf_error_t
usf_read_event(usf_file_t *file, usf_event_t *event)
{
    usf_error_t error = USF_ERROR_OK;

    E_ERROR(usf_internal_read(file, &event->type,
                              sizeof(usf_event_type_t)));
    if (error == USF_ERROR_EOF)
        goto ret_err;

    E_IF(event->type >= ARRAY_LEN(event_io),
	 USF_ERROR_FILE);

    E_ERROR(event_io[event->type].read(file, event));

ret_err:
    return error;
}

static usf_error_t
usf_read_trace(usf_file_t *file, usf_event_t *event)
{
    usf_error_t error = USF_ERROR_OK;

    event->type = USF_EVENT_TRACE;
    E_ERROR(read_access(file, &event->u.trace.access));

ret_err:
    return error;
}

usf_error_t
usf_read(usf_file_t *file, usf_event_t *event)
{
    if (!file || !event)
	return USF_ERROR_PARAM;

    if (file->header->flags & USF_FLAG_TRACE)
	return usf_read_trace(file, event);
    else
	return usf_read_event(file, event);
}

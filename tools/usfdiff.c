/*
 * Copyright (C) 2010-2011, David Eklov
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
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <uart/usf.h>

#define E_PRINT(fmt, args...)   \
    print_and_exit("%s:%i " fmt, __FILE__, __LINE__, args)

#define E_USF(e, name) do {                                     \
    usf_error_t _e = (e);                                       \
    if (_e != USF_ERROR_OK)                                     \
        E_PRINT("%s: %s\n", name, usf_strerror(_e));            \
} while (0)

#define E_NULL(e, name) do {                                    \
    __typeof__(e) _e = (e);                                     \
    if (!_e)                                                    \
        E_ERROR("%s: %s\n", name, strerror(errno));             \
} while (0)


typedef struct {
    char *file_name1;
    char *file_name2;
} args_t;

static char *usage_str = "Usage: usfdiff FILES\n";

static void __attribute__ ((format (printf, 1, 2)))
print_and_exit(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    exit(EXIT_FAILURE);
}

static void
parse_args(args_t *args, int argc, char **argv)
{
    if (argc != 3)
        print_and_exit("Missing argument\n\n%s\n", usage_str);

    args->file_name1 = argv[1];
    args->file_name2 = argv[2];
}


static int
comp_access(usf_access_t *a1, usf_access_t *a2)
{
    int c = 0;

    c += a1->pc   != a2->pc;
    c += a1->addr != a2->addr;
    c += a1->time != a2->time;
    c += a1->tid  != a2->tid;
    c += a1->len  != a2->len;
    c += a1->type != a2->type;

    return c;
}

static int
comp_sample(usf_event_sample_t *s1, usf_event_sample_t *s2)
{
    int c = 0;

    c += comp_access(&s1->begin, &s2->begin);
    c += comp_access(&s1->end, &s2->end);
    c += s1->line_size != s2->line_size;

    return c;
}

static int
comp_dangling(usf_event_dangling_t *d1, usf_event_dangling_t *d2)
{
    int c = 0;

    c += comp_access(&d1->begin, &d2->begin);
    c += d1->line_size != d2->line_size;

    return c;
}

static int
comp_burst(usf_event_burst_t *b1, usf_event_burst_t *b2)
{
    int c = 0;

    c += b1->begin_time != b2->begin_time;

    return c;
}

static int
comp_trace(usf_event_trace_t *t1, usf_event_trace_t *t2)
{
    int c = 0;

    c += comp_access(&t1->access, &t2->access);

    return c;
}

static int
comp_event(usf_event_t *e1, usf_event_t *e2)
{
    if (e1->type != e2->type)
        return 1;

    switch (e1->type) {
        case USF_EVENT_SAMPLE:
            return comp_sample(&e1->u.sample, &e2->u.sample);
        case USF_EVENT_DANGLING:
            return comp_dangling(&e1->u.dangling, &e2->u.dangling);
        case USF_EVENT_BURST:
            return comp_burst(&e1->u.burst, &e2->u.burst);
        case USF_EVENT_TRACE:
            return comp_trace(&e1->u.trace, &e2->u.trace);
        default:
            E_PRINT("%s\n", "file error");
    }

    /* Not reached */
}

static void
print_access(const usf_access_t *a)
{
    printf("[tid: %" PRIu16
           " pc: 0x%" PRIx64
           " addr: 0x%" PRIx64
           " time: %" PRIu64
           " len: %" PRIu16
           " type: %" PRIu8 " (%s)]",

           a->tid,
           a->pc,
           a->addr,
           a->time,
           a->len,
           a->type, usf_stratype(a->type));
}

static void
print_sample(usf_event_sample_t *s)
{
    printf("[SAMPLE] pc1: ");
    print_access(&s->begin);
    printf(", pc2: ");
    print_access(&s->end);
    printf(", ls: %u\n", (1U << s->line_size));
}

static void
print_dangling(usf_event_dangling_t *d)
{
    printf("[DANGLING] pc1: ");
    print_access(&d->begin);
    printf(", ls: %u\n", (1U << d->line_size));
}

static void
print_burst(usf_event_burst_t *b)
{
    printf("[BURST] begin_time: %" PRIu64 "\n", b->begin_time);
}

static void
print_trace(usf_event_trace_t *t)
{
    printf("[TRACE] pc1: ");
    print_access(&t->access);
    printf("\n");
}

static void
print_event(usf_event_t *e)
{
    switch (e->type) {
        case USF_EVENT_SAMPLE:
            print_sample(&e->u.sample);
            break;
        case USF_EVENT_DANGLING:
            print_dangling(&e->u.dangling);
            break;
        case USF_EVENT_BURST:
            print_burst(&e->u.burst);
            break;
        case USF_EVENT_TRACE:
            print_trace(&e->u.trace);
            break;
        default:
            E_PRINT("%s\n", "file error");
    }
}

#define USF_READ(usf_file, event) if (1) {      \
    error = usf_read(usf_file, event);          \
    if (error == USF_ERROR_EOF)                 \
        break;                                  \
    E_USF(error, "usf_read");                   \
}

int
main(int argc, char **argv)
{
    args_t args;
    usf_file_t *usf_file1;
    usf_file_t *usf_file2;
    usf_header_t header1;
    usf_header_t header2;
    usf_error_t error;

    parse_args(&args, argc, argv);

    error = usf_open(&usf_file1, args.file_name1);
    E_USF(error, "usf_open");
    error = usf_open(&usf_file2, args.file_name2);
    E_USF(error, "usf_open");

    /* XXX compare the headers */

    while (1) {
        usf_event_t event1;
        usf_event_t event2;

        USF_READ(usf_file1, &event1);
        USF_READ(usf_file2, &event2);

        if (comp_event(&event1, &event2)) {
            print_event(&event1);
            print_event(&event2);
            printf("\n");
        }
    }

    error = usf_close(usf_file1);
    E_USF(error, "usf_close");
    error = usf_close(usf_file2);
    E_USF(error, "usf_close");
    return 0;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */

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

#include <vector>
#include <queue>

#include <cstdlib>
#include <cstdio>
#include <cstdarg>

#include <uart/usf.h>

using namespace std;

#define E(e, name) do {                                         \
    usf_error_t __e = (e);                                      \
    if ((__e) != USF_ERROR_OK)                                  \
        print_and_exit("%s: %s\n", name, usf_strerror(__e));    \
} while (0)


static const char *usage_str = 
    "usfsort [OPTION...] INPUT OUTPUT";


struct args_t {
    char *ifile_name;
    char *ofile_name;
};

class comp_t {
public:
    comp_t(bool sort_on_pc1)
    : sort_on_pc1(sort_on_pc1) { }

    bool operator()(usf_event_t &e1, usf_event_t &e2) 
    {
        return get_time(e1) > get_time(e2);
    }

private:
    usf_atime_t get_time(usf_event_t &e)
    {
        usf_atime_t t;
        switch (e.type) {
            case USF_EVENT_SAMPLE:
                t = sort_on_pc1 ? e.u.sample.begin.time : e.u.sample.end.time;
                break;
            case USF_EVENT_DANGLING:
                t = e.u.dangling.begin.time;
                break;
            case USF_EVENT_BURST:
                t = e.u.burst.begin_time;
                break;
            case USF_EVENT_TRACE:
                t = e.u.trace.access.time;
                break;
        }
        return t;
    }

private:
    bool sort_on_pc1;
};

typedef priority_queue<usf_event_t, vector<usf_event_t>, comp_t> pqueue_t;

static void
print_and_exit(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    exit(EXIT_FAILURE);
}

static void
parse_args(args_t &args, int argc, char **argv)
{
    args.ifile_name = NULL;
    args.ofile_name = NULL;

    if (argc > 1)
        args.ifile_name = argv[1];

    if (argc > 2)
        args.ofile_name = argv[2];

    if (argc > 3)
        print_and_exit("%s\n", usage_str);
}

int
main(int argc, char **argv)
{
    args_t args;
    usf_file_t *usf_ifile;
    usf_file_t *usf_ofile;
    usf_header_t *header_in;
    usf_header_t header_out;
    usf_event_t event;
    usf_error_t error;

    parse_args(args, argc, argv);
    
    pqueue_t pqueue(comp_t(true));

    error = usf_open(&usf_ifile, args.ifile_name);
    E(error, "usf_open");

    error = usf_header((const usf_header_t **)&header_in, usf_ifile);
    E(error, "usf_header");
    header_out = *header_in;

    while ((error = usf_read(usf_ifile, &event)) == USF_ERROR_OK)
        pqueue.push(event);

    error = usf_close(usf_ifile);
    E(error, "usf_close");

    error = usf_create(&usf_ofile, args.ofile_name, &header_out);
    E(error, "usf_create");

    while (!pqueue.empty()) {
        error = usf_append(usf_ofile, &pqueue.top());
        E(error, "usf_append");
        pqueue.pop();
    }

    error = usf_close(usf_ofile);
    E(error, "usf_close");
    return 0;
}

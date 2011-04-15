/*
 * Copyright (C) 2010-2011, David Eklov
 * Copyright (C) 2011, Andreas Sandberg
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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/time.h>
#include <uart/usf.h>
#include "pin.H"

KNOB<string> knob_filename(KNOB_MODE_WRITEONCE,
                           "pintool", "o", "foo.usf", "Output filename");
KNOB<UINT64> knob_begin_addr(KNOB_MODE_WRITEONCE,
                             "pintool", "b", "0", "Start tracing at this address");
KNOB<UINT64> knob_end_addr(KNOB_MODE_WRITEONCE,
                           "pintool", "e", "0", "Stop tracing at this address");
KNOB<BOOL> knob_detach(KNOB_MODE_WRITEONCE,
                       "pintool", "d", "0", "Stop pin at stop address");
KNOB<BOOL> knob_bzip2(KNOB_MODE_WRITEONCE,
                      "pintool", "c", "0", "Enable BZip2 compression");

static int tracing;
static unsigned long begin_addr; 
static unsigned long end_addr;

static usf_file_t *usf_file;
static usf_atime_t usf_time;

static VOID fini(INT32 code, VOID *v);

#define DEF_LOG(_name, _type)                                   \
    static VOID                                                 \
    _name(VOID *pc, VOID *addr, ADDRINT size, THREADID tid)     \
    {                                                           \
        usf_event_t event;                                      \
        usf_access_t *access;                                   \
                                                                \
        event.type = USF_EVENT_TRACE;                           \
        access = &event.u.trace.access;                         \
                                                                \
        access->pc   = (usf_addr_t)pc;                          \
        access->addr = (usf_addr_t)addr;                        \
        access->time = usf_time++;                              \
        access->tid  = (usf_tid_t)tid;                          \
        access->len  = size;                                    \
        access->type = _type;                                   \
                                                                \
        usf_append(usf_file, &event);                           \
    }

DEF_LOG(log_rd, USF_ATYPE_RD)
DEF_LOG(log_wr, USF_ATYPE_WR)

static ADDRINT
is_tracing()
{
    return tracing;
}

static VOID
instruction(INS ins, VOID *not_used)
{
    if (begin_addr && INS_Address(ins) == begin_addr)
        tracing = 1;
    if (end_addr && INS_Address(ins) == end_addr) {
        tracing = 0;
        if (knob_detach) {
            fini(0, NULL);
            exit(0);
        }
    }

    if (INS_IsMemoryRead(ins)) {
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)is_tracing, IARG_END);
        INS_InsertThenCall(ins, IPOINT_BEFORE,
                           (AFUNPTR)log_rd,
                           IARG_INST_PTR,
                           IARG_MEMORYREAD_EA,
                           IARG_MEMORYREAD_SIZE,
                           IARG_THREAD_ID,
                           IARG_END);
    }
    if (INS_IsMemoryWrite(ins)) {
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)is_tracing, IARG_END);
        INS_InsertThenCall(ins, IPOINT_BEFORE,
                           (AFUNPTR)log_wr,
                           IARG_INST_PTR,
                           IARG_MEMORYWRITE_EA,
                           IARG_MEMORYWRITE_SIZE,
                           IARG_THREAD_ID,
                           IARG_END);
    }
}

static int
init(int argc, char *argv[])
{
    const char *filename = knob_filename.Value().c_str();
    usf_header_t header;
    struct timeval tv;
    int target_argc = argc;
    char **target_argv = argv;

    memset(&header, 0, sizeof(header));
    
    begin_addr = knob_begin_addr;
    if (!begin_addr)
        tracing = 1;
    end_addr = knob_end_addr;

    header.version = USF_VERSION_CURRENT;
    header.compression = knob_bzip2.Value() ? USF_COMPRESSION_BZIP2 : USF_COMPRESSION_NONE;
    header.flags = USF_FLAG_NATIVE_ENDIAN | USF_FLAG_TRACE | USF_FLAG_DELTA;

    if (gettimeofday(&tv, NULL) == 0)
	header.time_begin = tv.tv_sec;
    else
	cerr << "Warning: Failed to get time of day, "
	     << "information not included in trace file." << endl;

    for (int i = 0; i < argc; i++) {
	if (!strcmp("--", argv[i])) {
	    target_argc = argc - i - 1;
	    target_argv = argv + i + 1;
	}
    }
    header.argc = target_argc;
    header.argv = target_argv;

    return usf_create(&usf_file, filename, &header) == USF_ERROR_OK ? 0 : -1;
}

static VOID
fini(INT32 code, VOID *v)
{
    usf_close(usf_file);
}

static int
usage()
{
    cerr <<
        "This tool is a PIN tool to generate USF trace files.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}

int
main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv))
        return usage();

    if (init(argc, argv))
        return -1;

    INS_AddInstrumentFunction(instruction, 0);
    PIN_AddFiniFunction(fini, 0);

    PIN_StartProgram();
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

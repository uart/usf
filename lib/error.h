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

#ifndef ERROR_H
#define ERROR_H
#include <stdio.h>
#include <uart/usf.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef ENABLE_DEBUG_LOG
#define DBGLOG(error_, expr_)                                   \
    fprintf(stderr, "Error: %s: %s: %d: %s\n",                  \
            usf_strerror(error_), __FILE__, __LINE__, expr_)
#else
#define DBGLOG(error_, expr_)
#endif

#define E_IF(expr, err)					\
    do {						\
        if (expr) {					\
            error = err;				\
	    DBGLOG(err, #expr);				\
	    goto ret_err;				\
        }						\
    } while (0)

#define E_NULL(expr, err) E_IF(!(expr), err)


#define E_ERROR(expr)                                           \
    do {                                                        \
        error = expr;                                           \
        if (error != USF_ERROR_OK && error != USF_ERROR_EOF) {  \
            DBGLOG(error, #expr);                               \
            goto ret_err;                                       \
        }                                                       \
    } while (0)


#define E_IF_IO(file, expr)						\
    do {								\
        if (expr) {							\
	    error = feof(file) ? USF_ERROR_EOF : USF_ERROR_SYS;	        \
            DBGLOG(error, #expr);                                       \
	    goto ret_err;						\
        }								\
    } while(0)

#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */

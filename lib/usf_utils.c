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

#include "usf_priv.h"

static const char *usf_errors[] = {
    "No error",
    "Invalid parameter",
    "Operating system error",
    "Memory allocation failure",
    "End of file",
    "File format error",
    "Unsupported option",
};

static const char *usf_compressions[] = {
    "None",
    "BZip2",
};

static const char *usf_atypes[] = {
    "RD",
    "WR",
    "R&W",

    "PF_NTA",
    "PF_T0",
    "PF_T1",
    "PF_T2",
    "PF",
    "PF_W",

    "UNKNOWN",

    "INSTR",
};

const char *
usf_strerror(usf_error_t error)
{
    if (error < ARRAY_LEN(usf_errors))
	return usf_errors[error];
    else
	return "Unknown error";
}

const char *
usf_strcompr(usf_compression_t compr)
{
    if (compr < ARRAY_LEN(usf_compressions))
	return usf_compressions[compr];
    else
	return "Unknown";
}

const char *
usf_stratype(usf_atype_t atype)
{
    if (atype < ARRAY_LEN(usf_atypes))
	return usf_atypes[atype];
    else
	return "Unknown";
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */

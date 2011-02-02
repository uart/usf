bin_PROGRAMS = usfdump usf2usf usfsort usfcat usfstats usf2trace \
               usfresampler usfdiff usfsplit usfgentrace

CPPFLAGS = -I $(top_srcdir)/include
LDADD = ../lib/libusf.a

# XXX -lm is only needed by usfresampler
LIBS += -lm


usfdump_SOURCES = usfdump.c
usf2usf_SOURCES = usf2usf.c
usfsort_SOURCES = usfsort.cc
usfcat_SOURCES = usfcat.c
usfstats_SOURCES = usfstats.c
usf2trace_SOURCES = usf2trace.c
usfresampler_SOURCES = usfresampler.c
usfdiff_SOURCES = usfdiff.c
usfsplit_SOURCES = usfsplit.c
usfgentrace_SOURCES = usfgentrace.c

SHELL=/bin/sh
BENCHMARK=is
BENCHMARKU=IS
CFLAGS_LL=-emit-llvm -S -w -fno-use-cxa-atexit
OPTFLAGS=-O3

include ../config/make.def

include ../sys/make.common

OBJS = is.ll
COMMON_OBJS = ${COMMON}/c_print_results.o \
       ${COMMON}/c_timers.o \
       ${COMMON}/c_wtime.o


${PROGRAM}: config ${COMMON_OBJS} ${OBJS} LLFI
	${CLINK} ${CLINKFLAGS} -o ${PROGRAM} ${COMMON_OBJS} ${OBJS} ${C_LIB}

.c.o:
	${CCOMPILE} $<

is.ll:             is.c  npbparams.h


clean:
	- rm -f *.o *~ mputil*
	- rm -f npbparams.h core
	- rm -rf *.ll *.bc llfi* compile*txt

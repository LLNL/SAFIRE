SHELL=/bin/sh
BENCHMARK=cg
BENCHMARKU=CG
# Only for CG we allow O1, when invoking opt, llfi breaks...
CFLAGS_LL=-emit-llvm -S -O1 -w -fno-use-cxa-atexit
OPTFLAGS=

include ../config/make.def

include ../sys/make.common

OBJS = cg.ll
COMMON_OBJS = ${COMMON}/print_results.o  \
       ${COMMON}/${RAND}.o \
       ${COMMON}/c_timers.o \
       ${COMMON}/wtime.o



${PROGRAM}: config ${COMMON_OBJS} ${OBJS} LLFI
	${CLINK} ${CLINKFLAGS} -o ${PROGRAM} ${OBJS} ${COMMON_OBJS} ${C_LIB}

.c.o:
	${CCOMPILE} $<

cg.ll : cg.c globals.h npbparams.h

clean:
	- rm -f *.o *~ 
	- rm -f npbparams.h core
	- rm -rf *.ll *.bc llfi* compile*txt




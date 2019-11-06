SHELL=/bin/sh 
BENCHMARK=mg
BENCHMARKU=MG
CFLAGS_LL=-emit-llvm -S -w -fno-use-cxa-atexit
OPTFLAGS=-O3

include ../config/make.def

include ../sys/make.common

OBJS = mg.ll
COMMON_OBJS = ${COMMON}/print_results.o  \
       ${COMMON}/${RAND}.o \
       ${COMMON}/c_timers.o \
       ${COMMON}/wtime.o


${PROGRAM}: config ${COMMON_OBJS} ${OBJS} LLFI
	${CLINK} ${CLINKFLAGS} -o ${PROGRAM} ${COMMON_OBJS} ${OBJS} ${C_LIB}

.c.o:
	${CCOMPILE} $<

mg.ll:		mg.c globals.h npbparams.h

clean:
	- rm -f *.o *~ 
	- rm -f npbparams.h core
	- rm -rf *.ll *.bc llfi* compile*txt

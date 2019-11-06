SHELL=/bin/sh
BENCHMARK=ft
BENCHMARKU=FT
CFLAGS_LL=-emit-llvm -S -w -fno-use-cxa-atexit
OPTFLAGS=-O3

include ../config/make.def

include ../sys/make.common

OBJS = appft.ll auxfnct.ll fft3d.ll mainft.ll verify.ll
COMMON_OBJS = ${COMMON}/${RAND}.o ${COMMON}/print_results.o \
       ${COMMON}/c_timers.o ${COMMON}/wtime.o

${PROGRAM}: config ${COMMON_OBJS} ${OBJS} LLFI
	${CLINK} ${CLINKFLAGS} -o ${PROGRAM} ${COMMON_OBJS} ${OBJS} ${C_LIB}

.c.o:
	${CCOMPILE} $<

appft.ll:	appft.c  global.h npbparams.h
auxfnct.ll:	auxfnct.c  global.h npbparams.h
fft3d.ll:	fft3d.c  global.h npbparams.h
mainft.ll:	mainft.c  global.h npbparams.h
verify.ll:	verify.c  global.h npbparams.h

clean:
	- rm -f *.o *~ mputil*
	- rm -f ft npbparams.h core
	- rm -rf *.ll *.bc llfi* compile*txt

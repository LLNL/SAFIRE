SHELL=/bin/sh
BENCHMARK=dc
BENCHMARKU=DC
CFLAGS_LL=-emit-llvm -S -w -fno-use-cxa-atexit
OPTFLAGS=-O3

include ../config/make.def

include ../sys/make.common

OBJS = adc.ll dc.ll extbuild.ll rbt.ll jobcntl.ll
COMMON_OBJS = ${COMMON}/c_print_results.o  \
	${COMMON}/c_timers.o ${COMMON}/c_wtime.o


# npbparams.h is provided for backward compatibility with NPB compilation
# header.h: npbparams.h

${PROGRAM}: config ${COMMON_OBJS} ${OBJS} LLFI
	${CLINK} ${CLINKFLAGS} -o ${PROGRAM} ${COMMON_OBJS} ${OBJS} ${C_LIB}

.c.o:
	$(CCOMPILE) $<

adc.ll:      adc.c npbparams.h
dc.ll:       dc.c adcc.h adc.h macrodef.h npbparams.h
extbuild.ll: extbuild.c adcc.h adc.h macrodef.h npbparams.h
rbt.ll:      rbt.c adcc.h adc.h rbt.h macrodef.h npbparams.h
jobcntl.ll:  jobcntl.c adcc.h adc.h macrodef.h npbparams.h

clean:
	- rm -f *.o 
	- rm -f npbparams.h core
	- rm -f {../,}ADC.{logf,view,dat,viewsz,groupby,chunks}.* 
	- rm -rf *.ll *.bc llfi* compile*txt


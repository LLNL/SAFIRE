SHELL=/bin/sh
BENCHMARK=ua
BENCHMARKU=UA
#CFLAGS_LL=-emit-llvm -O1 -S -w -fno-use-cxa-atexit
CFLAGS_LL=-emit-llvm -O1 -S -w -fno-use-cxa-atexit
# have to disable opt to make it work
OPTFLAGS=-disable-opt 

include ../config/make.def

include ../sys/make.common

OBJS = ua.ll convect.ll diffuse.ll adapt.ll move.ll mason.ll \
       precond.ll utils.ll transfer.ll verify.ll  setup.ll
COMMON_OBJS = ${COMMON}/print_results.o ${COMMON}/c_timers.o ${COMMON}/wtime.o


# npbparams.h is included by header.h
# The following rule should do the trick but many make programs (not gmake)
# will do the wrong thing and rebuild the world every time (because the
# mod time on header.h is not changed. One solution would be to 
# touch header.h but this might cause confusion if someone has
# accidentally deleted it. Instead, make the dependency on npbparams.h
# explicit in all the lines below (even though dependence is indirect). 

# header.h: npbparams.h

${PROGRAM}: config ${COMMON_OBJS} ${OBJS} LLFI
	${CLINK} ${CLINKFLAGS} -o ${PROGRAM} ${COMMON_OBJS} ${OBJS} ${C_LIB}

.c.o:
	${CCOMPILE} $<

ua.ll:        ua.c       header.h npbparams.h
setup.ll:     setup.c    header.h npbparams.h
convect.ll:   convect.c  header.h npbparams.h
adapt.ll:     adapt.c    header.h npbparams.h
move.ll:      move.c     header.h npbparams.h
diffuse.ll:   diffuse.c  header.h npbparams.h
mason.ll:     mason.c    header.h npbparams.h
precond.ll:   precond.c  header.h npbparams.h
transfer.ll:  transfer.c header.h npbparams.h
utils.ll:     utils.c    header.h npbparams.h
verify.ll:    verify.c   header.h npbparams.h

clean:
	- rm -f *.o *~ mputil*
	- rm -f npbparams.h core
	- rm -rf *.ll llfi* compile*txt

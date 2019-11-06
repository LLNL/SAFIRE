SHELL=/bin/sh
BENCHMARK=lu
BENCHMARKU=LU
VEC=
CFLAGS_LL=-emit-llvm -S -w -fno-use-cxa-atexit
OPTFLAGS=-O3

include ../config/make.def

include ../sys/make.common

OBJS = lu.ll read_input.ll \
       domain.ll setcoeff.ll setbv.ll exact.ll setiv.ll \
       erhs.ll ssor$(VEC).ll rhs$(VEC).ll l2norm.ll \
       jacld.ll blts$(VEC).ll jacu.ll buts$(VEC).ll error.ll \
       pintgr.ll verify.ll 
COMMON_OBJS = ${COMMON}/print_results.o \
       ${COMMON}/c_timers.o ${COMMON}/wtime.o


# npbparams.h is included by applu.incl
# The following rule should do the trick but many make programs (not gmake)
# will do the wrong thing and rebuild the world every time (because the
# mod time on header.h is not changed. One solution would be to 
# touch header.h but this might cause confusion if someone has
# accidentally deleted it. Instead, make the dependency on npbparams.h
# explicit in all the lines below (even though dependence is indirect). 

# applu.incl: npbparams.h

${PROGRAM}: config
	@if [ x$(VERSION) = xvec ] ; then	\
		${MAKE} VEC=_vec exec;		\
	elif [ x$(VERSION) = xVEC ] ; then	\
		${MAKE} VEC=_vec exec;		\
	else					\
		${MAKE} exec;			\
	fi

exec: $(COMMON_OBJS) $(OBJS) LLFI
	${CLINK} ${CLINKFLAGS} -o ${PROGRAM} $(COMMON_OBJS) ${OBJS} ${C_LIB}

.c.o:
	${CCOMPILE} $<

lu.ll:		lu.c applu.incl npbparams.h
blts$(VEC).ll:	blts$(VEC).c
buts$(VEC).ll:	buts$(VEC).c	
erhs.ll:		erhs.c applu.incl npbparams.h
error.ll:	error.c applu.incl npbparams.h
exact.ll:	exact.c applu.incl npbparams.h
jacld.ll:	jacld.c applu.incl npbparams.h
jacu.ll:		jacu.c applu.incl npbparams.h
l2norm.ll:	l2norm.c
pintgr.ll:	pintgr.c applu.incl npbparams.h
read_input.ll:	read_input.c applu.incl npbparams.h
rhs$(VEC).ll:	rhs$(VEC).c applu.incl npbparams.h
setbv.ll:	setbv.c applu.incl npbparams.h
setiv.ll:	setiv.c applu.incl npbparams.h
setcoeff.ll:	setcoeff.c applu.incl npbparams.h
ssor$(VEC).ll:	ssor$(VEC).c applu.incl npbparams.h
domain.ll:	domain.c applu.incl npbparams.h
verify.ll:	verify.c applu.incl npbparams.h

clean:
	- /bin/rm -f npbparams.h
	- /bin/rm -f *.o *~
	- rm -rf *.ll *.bc llfi* compile*txt

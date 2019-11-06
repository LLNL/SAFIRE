SHELL=/bin/sh
BENCHMARK=bt
BENCHMARKU=BT
VEC=
CFLAGS_LL=-emit-llvm -S -w -fno-use-cxa-atexit
OPTFLAGS=-O3

include ../config/make.def

include ../sys/make.common

OBJS = bt.ll  initialize.ll exact_solution.ll exact_rhs.ll \
       set_constants.ll adi.ll  rhs.ll      \
       x_solve$(VEC).ll y_solve$(VEC).ll solve_subs.ll  \
       z_solve$(VEC).ll add.ll error.ll verify.ll
COMMON_OBJS = ${COMMON}/print_results.o ${COMMON}/c_timers.o ${COMMON}/wtime.o


# npbparams.h is included by header.h
# The following rule should do the trick but many make programs (not gmake)
# will do the wrong thing and rebuild the world every time (because the
# mod time on header.h is not changed. One solution would be to 
# touch header.h but this might cause confusion if someone has
# accidentally deleted it. Instead, make the dependency on npbparams.h
# explicit in all the lines below (even though dependence is indirect). 

# header.h: npbparams.h

${PROGRAM}: config
	@if [ x$(VERSION) = xvec ] ; then	\
		${MAKE} VEC=_vec exec;		\
	elif [ x$(VERSION) = xVEC ] ; then	\
		${MAKE} VEC=_vec exec;		\
	else					\
		${MAKE} exec;			\
	fi

exec: $(COMMON_OBJS) $(OBJS) LLFI
	${CLINK} ${CLINKFLAGS} -o ${PROGRAM} ${COMMON_OBJS} ${OBJS} ${C_LIB}

.c.o:
	${CCOMPILE} $<


bt.ll:             bt.c  header.h npbparams.h
initialize.ll:     initialize.c  header.h npbparams.h
exact_solution.ll: exact_solution.c  header.h npbparams.h
exact_rhs.ll:      exact_rhs.c  header.h npbparams.h
set_constants.ll:  set_constants.c  header.h npbparams.h
adi.ll:            adi.c  header.h npbparams.h
rhs.ll:            rhs.c  header.h npbparams.h
x_solve$(VEC).ll:  x_solve$(VEC).c  header.h work_lhs$(VEC).h npbparams.h
y_solve$(VEC).ll:  y_solve$(VEC).c  header.h work_lhs$(VEC).h npbparams.h
z_solve$(VEC).ll:  z_solve$(VEC).c  header.h work_lhs$(VEC).h npbparams.h
solve_subs.ll:     solve_subs.c  npbparams.h
add.ll:            add.c  header.h npbparams.h
error.ll:          error.c  header.h npbparams.h
verify.ll:         verify.c  header.h npbparams.h

clean:
	- rm -f *.o *~ mputil*
	- rm -f  npbparams.h core
	- rm -rf *.ll *.bc llfi* compile*txt 

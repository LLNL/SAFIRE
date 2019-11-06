#===============================================================================
# User Options
#===============================================================================

COMPILER    = clang
OPTIMIZE    = yes
OPENMP 	    = no
DEBUG       = no
PROFILE     = no
MPI         = no
PAPI        = no
VEC_INFO    = no
VERIFY      = yes
BENCHMARK   = no
BINARY_DUMP = no
BINARY_READ = no

#===============================================================================
# Program name & source code list
#===============================================================================

program = XSBench

source = \
Main.c \
io.c \
CalculateXS.c \
GridInit.c \
XSutils.c \
Materials.c

#obj = $(source:.c=.o)
obj = $(source:.c=.ll)

#===============================================================================
# Sets Flags
#===============================================================================

# Standard Flags
CFLAGS := -std=gnu99 

# Linker Flags
LDFLAGS = -lm

# Regular gcc Compiler
ifeq ($(COMPILER),gnu)
  CC = gcc
ifeq ($(OPENMP),yes)
  CFLAGS += -fopenmp -DOPENMP
endif
endif

# CLANG compiler
ifeq ($(COMPILER),clang)
  CC = $(LLFI_BUILD_ROOT)/llvm/bin/clang
ifeq ($(OPENMP),yes)
  CFLAGS += -fopenmp -DOPENMP
endif
endif


# Intel Compiler
ifeq ($(COMPILER),intel)
  CC = icc
ifeq ($(OPENMP),yes)
  CFLAGS += -openmp -DOPENMP
endif
endif

# BG/Q gcc Cross-Compiler
ifeq ($(MACHINE),bluegene)
  CC = mpicc
endif

# Debug Flags
ifeq ($(DEBUG),yes)
  CFLAGS += -g
  LDFLAGS  += -g
endif

# Profiling Flags
ifeq ($(PROFILE),yes)
  CFLAGS += -pg
  LDFLAGS  += -pg
endif

# Optimization Flags
ifeq ($(OPTIMIZE),yes)
  CFLAGS += -O3
endif

# Compiler Vectorization (needs -O3 flag) information
ifeq ($(VEC_INFO),yes)
  CFLAGS += -ftree-vectorizer-verbose=6
endif

# PAPI source (you may need to provide -I and -L pointing
# to PAPI depending on your installation
ifeq ($(PAPI),yes)
  source += papi.c
  CFLAGS += -DPAPI
  #CFLAGS += -I/soft/apps/packages/papi/papi-5.1.1/include
  #LDFLAGS += -L/soft/apps/packages/papi/papi-5.1.1/lib -lpapi
  LDFLAGS += -lpapi
endif

# MPI
ifeq ($(MPI),yes)
  CC = mpicc
  CFLAGS += -DMPI
endif

# Verification of results mode
ifeq ($(VERIFY),yes)
  CFLAGS += -DVERIFICATION
endif

# Adds outer 'benchmarking' loop to do multiple trials for
# 1 < threads <= max_threads
ifeq ($(BENCHMARK),yes)
  CFLAGS += -DBENCHMARK
endif

# Binary dump for file I/O based initialization
ifeq ($(BINARY_DUMP),yes)
  CFLAGS += -DBINARY_DUMP
endif

# Binary read for file I/O based initialization
ifeq ($(BINARY_READ),yes)
  CFLAGS += -DBINARY_READ
endif


#===============================================================================
# Targets to Build
#===============================================================================

$(program): $(obj) XSbench_header.h
	$(LLFI_BUILD_ROOT)/llvm/bin/llvm-link -S $(obj) -o $(program).all.ll
	${LLFI_BUILD_ROOT}/llvm/bin/opt -O3 -S $(program).all.ll -o $(program).ll
	${LLFI_BUILD_ROOT}/llfi/bin/instrument --dir 'llfi' --cflags "$(CFLAGS)" --cc c --readable $(program).ll
	$(CC) $(CFLAGS) $(program).ll -o $@ $(LDFLAGS)

%.ll: %.c
	$(CC) $(CFLAGS) -emit-llvm -S -fno-use-cxa-atexit $< -o $@

clean:
	rm -rf $(program) $(obj) *.ll llfi*

edit:
	vim -p $(source) papi.c XSbench_header.h

run:
	./$(program)

bgqrun:
	qsub -t 10 -n 1 -O test XSBench

#default build suggestion of MPI + OPENMP with gcc on Livermore machines you might have to change the compiler name

SHELL = /bin/sh
.SUFFIXES: .cc .ll

LULESH_EXEC = lulesh2.0

MPI_INC = /opt/local/include/openmpi
MPI_LIB = /opt/local/lib

SERCXX = $(LLFI_BUILD_ROOT)/llvm/bin/clang++
MPICXX = mpig++ -DUSE_MPI=1
CXX = $(SERCXX)
CXXFLAGS_LL = -emit-llvm -S -w -fno-use-cxa-atexit -DUSE_MPI=0
LINKER_LL=$(LLFI_BUILD_ROOT)/llvm/bin/llvm-link

SOURCES2.0 = \
	lulesh.cc \
	lulesh-comm.cc \
	lulesh-viz.cc \
	lulesh-util.cc \
	lulesh-init.cc
OBJECTS2.0 = $(SOURCES2.0:.cc=.ll)

#Default build suggestions with OpenMP for g++
#CXXFLAGS = -g -O3 -fopenmp -I. -Wall
CXXFLAGS = -g -O3 -I. -Wall
#LDFLAGS = -g -O3 -fopenmp
LDFLAGS = -g -O3

#Below are reasonable default flags for a serial build
#CXXFLAGS = -g -O3 -I. -Wall
#LDFLAGS = -g -O3 

#common places you might find silo on the Livermore machines.
#SILO_INCDIR = /opt/local/include
#SILO_LIBDIR = /opt/local/lib
#SILO_INCDIR = ./silo/4.9/1.8.10.1/include
#SILO_LIBDIR = ./silo/4.9/1.8.10.1/lib

#If you do not have silo and visit you can get them at:
#silo:  https://wci.llnl.gov/codes/silo/downloads.html
#visit: https://wci.llnl.gov/codes/visit/download.html

#below is and example of how to make with silo, hdf5 to get vizulization by default all this is turned off.  All paths are Livermore specific.
#CXXFLAGS = -g -DVIZ_MESH -I${SILO_INCDIR} -Wall -Wno-pragmas
#LDFLAGS = -g -L${SILO_LIBDIR} -Wl,-rpath -Wl,${SILO_LIBDIR} -lsiloh5 -lhdf5

.cc.ll: lulesh.h
	@echo "Building $<"
	$(CXX) -c $(CXXFLAGS_LL) -o $@  $<

all: $(LULESH_EXEC)

lulesh2.0: $(LULESH_EXEC).ll
	${LLFI_BUILD_ROOT}/llfi/bin/instrument --dir 'llfi' --cflags "$(CXXFLAGS)" --cc "c++" --readable $<
	
$(LULESH_EXEC).ll: $(OBJECTS2.0)
	@echo "Linking"
	$(LINKER_LL) -S $(OBJECTS2.0) -o $@
	$(LINKER_LL) -S $(OBJECTS2.0) -o $(LULESH_EXEC).all.ll
	${LLFI_BUILD_ROOT}/llvm/bin/opt -O3 $(LULESH_EXEC).all.ll -o $@

clean:
	/bin/rm -f *.o *~ $(OBJECTS) $(LULESH_EXEC)
	/bin/rm -rf *.dSYM
	rm -rf *.ll llfi*

tar: clean
	cd .. ; tar cvf lulesh-2.0.tar LULESH-2.0 ; mv lulesh-2.0.tar LULESH-2.0


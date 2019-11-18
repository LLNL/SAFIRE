# SAFIRE : Scalable and Accurate Fault Injection for Parallel Multi-threaded Applications

SAFIRE is a fault injection framework, descending from
[REFINE](https://github.com/ggeorgakoudis/REFINE), that supports injecting
bit-flip faults in both serial and multi-threaded programs. Also, it supports
injecting faults to multi-process, distributed execution but this is
experimental at the moment.

SAFIRE includes a modified LLVM compiler backend for x86 to instrument and
inject bit-flip faults on machine instructions. The backend implements several
optimizations for fast instrumentation and injection that make SAFIRE the
fastest and most accurate tool for fault injection so far.  Compiling with
SAFIRE produces an instrumented binary ready to interface with a dynamic
library that implements an API of function hooks. Function hooks get
information on the executed instruction and may trigger fault injection. 
Dynamic libraries can implement any instruction-based fault model to select
which instruction(s) to inject to, which operand(s), and which bit(s) to flip.

## Getting Started

### Repo directory structure

The main directories of the repo are:

* **llvm-3.9.0**, which contains the modified LLVM compiler
* **libinject**, which contains implementations of a single fault model for serial, multi-threaded, and multi-process distributed execution (experimental)

The repo contains also a reference directory of the paper on SAFIRE presented at IPDPS'19, named _ipdps19_. Its sub-directories are:

* _pinfi_, contains a fault injection tool based on Intel PIN used to compare with SAFIRE
* _programs_, contains several programs used in experimentation, sub-directories of each program are:
   - _golden_, program in vanilla version with no modification
   - _llfi_, programs that their build process is modified to use the LLFI tool (https://github.com/DependableSystemsLab/LLFI) for fault injection
   - _pinfi_, like golden, programs with no modifications for building using PINFI, the purpose of the sub-directory is store the output of experiments
   - _refine_, programs that their building process is modified to use REFINE for fault injection
* _results_, contains .eps figures of the accuracy and performance results published in the paper
* _scripts_, contains the scripts used for running experiments, and post-processing for figure and table creation


### Build the SAFIRE LLVM compiler
1. Clone the repo

`git clone <repo url>`

2. Change directory to llvm-3.9.0

`cd llvm-3.9.0`

3. Download clang-3.9.0 (http://releases.llvm.org/3.9.0/cfe-3.9.0.src.tar.xz) and decompress it in the llvm-3.9.0/tools/ sub-directory
```
wget -P tools/ http://releases.llvm.org/3.9.0/cfe-3.9.0.src.tar.xz
tar -C tools/ -xf tools/cfe-3.9.0.src.tar.xz 
```

4. Create a directory for building, e.g., BUILD

`mkdir BUILD`

5. Change to the building directory

`cd BUILD`

6. Run cmake to boostrap the build proces and set the installation directory, e.g.,

`cmake -DCMAKE_INSTALL_PREFIX=$HOME/opt/safire -DLLVM_TARGETS_TO_BUILD=X86 -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DLLVM_OPTIMIZED_TABLEGEN=ON -DLLVM_ENABLE_DOXYGEN=OFF ..`

7. Run the build program to process cmake generated build files, e.g.,

`make -j$(nproc)`

8. Install the SAFIRE LLVM compiler binaries, e.g.,

`make install`

### Use the SAFIRE LLVM compiler
1. Set the environment path to include the installation directory, e.g.,

`export PATH="$HOME/opt/safire/bin:$PATH"`

2. SAFIRE extends the LLVM compiler with Fault Injection (FI) flags. Those are:

|     Flag         |                 Description                       |
| -----------------| ------------------------------------------------- |
| -fi              | Enable SAFIRE instrumentation and FI in the LLVM backend       |
| -fi-ff           | Enable the _fast-forwarding_ optimization for instrumentation and injection. **Should always enable it for significant speedup, disabling it is there only for comparison** |
| -fi-funcs        | Comma separated list of functions to target for instrumentation and injection. Setting to "*" selects all |
| -fi-funcs-excl   | Comma separated list of functions to **exclude** from instrumentation and injection |
| -fi-inst-types   | Comma separated list of instruction types to target for FI, possible values are: _frame, control, data_. Setting to "*" selects all |
| -fi-reg-types    | comma separated list of register types to be possible FI targets, possible types are: _src, dst_. Setting to "*" selects all

To include SAFIRE's instrumentation in the compilation process, you need to include the SAFIRE FI flags in the flags given to the compiler driver, such as `clang`. For example, enabling SAFIRE within a Makefile of C compilation extends the 
variable CFLAGS as:

`CFLAGS += -mllvm -fi -mllvm -fi-ff -mllvm -fi-funcs="*" -mllvm -fi-inst-types="*" -mllvm fi-reg-types="dst"`

Those flags enable fault injection with the fast-forwarding optimization, targeting all functions and instructions to inject faults to destination registers.

The following is the same example but invoking `clang` from the command line:

`clang -O3 -mllvm -fi -mllvm -fi-ff -mllvm -fi-funcs="*" -mllvm -fi-inst-types="*" -mllvm fi-reg-types="dst"`

For more examples, see programs in the `programs/safire` directory of the repo.

3. Compiling with SAFIRE **requires** linking with a library that implements routines hooks emitted by SAFIRE instrumentation. The prototypes of those routines and their function is:

`void selMBB(uint64_t *ret, uint64_t num_insts)`
The instrumented program calls this routine on entry to a (Machine) Basic Block of machine instructions (a Basic Block is a sequence of instructions that execute indivisibly). This is the
default instrumentation mode enabled when the program starts execution.

The variable **num_insts** is input and has the number of instructions in this basic block.
The variable **ret** is output pointing to a memory location. The value that the routine stores in this memory location guides the instrumentation in the program; there are three possibilities:
* *ret = 0, execution continues with Basic Block instrumentation
* *ret = 1, execution continues with detailed per-instruction instrumentation
* *ret = 2, execution continues with instrumentation disabled, there will not be any further calls to hooks nor any instrumentation overhead

The typical use of selMBB is to count the number of dynamic instructions executed so far to decide, based on fault model, whether fault injection should happen to one of the instructions in this
basic block. If no, then *ret=0 instructs execution to continue execution without instrumentation until the next basic block. If yes, then *ret=1 instructions execution to continue execution with 
per-instruction instrumentation that steps every instruction of the basic block until the target is found and the fault is injected. If there are no more faults to inject, by the fault model, *ret=2 disables 
instrumentation and avoids any overhead from that point on.


`void selInst(uint64_t *ret, uint8_t *instr_str)`
The instrumented program calls this routine for each instruction, when per-instruction instrumentation is enabled.

The variable **instr_str** is input and has a textual, C-string representation of the instruction to execute in the next step.
The variable **ret** is output pointing to a memory location. The value that the routine stores in this memory locations guides fault injection; there are two possibilities:
* *ret = 0, execution continues to the next instruction without fault injection
* *ret = 1, execution continues but after this instruction executes, the instrumented binary will invoke the fault injection routine hook doInject, discussed next.

`void doInject(unsigned num_ops, uint64_t *op, uint64_t *size, uint8_t *bitmask)`
If selInstr sets *ret = 1, the instrumented binary calls doInject right *after* the instruction in selInst has executed. The routine
doInject can change the value of any operand using a bitmask to inject bit-flips.

The variable **num\_ops** is input and has the number of operands for the instruction.
The variable **op** is output pointing to a memory location. The value the routine stores in this memory location is the identifier
of the operand to inject a fault, valid values are 0..num_ops-1.
The variable **size** is input pointing to an array of length num_ops that stores the size in bytes of operands, indexed by the 
operand identifier. This is helper data to communicate the size of operands.
The variable **bitmask** is output and determines the bitmask to apply to the chosen operand *op. It is a pointer to a byte array
that has been allocated by instrumentation storing the bitmask in least significant bit first order (little-endian). 
A value of '1' in bit position causes injection to flip the bit of the operand at that position.

The instrumented binary **must** link to a library that implements those function hooks.
There are examples of libraries implementing the single fault model for serial and parallel execution under the directory `libinject`.

### Run a fault-injection experiment using SAFIRE and the single-fault model library

The FI library we provide needs a dynamic instruction count to pool a random instruction to inject fault. For that, the library reads the dynamic target instruction counter for the file `fi-inscount.txt`. If the file is missing, our library implementation performs a boostrap run that does the counting without injecting faults. There are different example libraries depending on the whether targeting serial, multi-threaded, or multi-process (experimental) execution.
The format of the file `fi-inscount.txt` for multi-threaded execution is:
```
thread=X, fi_index=N
...
fi_index=M
```
where X is the thread id, fi_index in the same line is the number of dynamic instructions thread X executed, and the final fi_index is the total dynamic instructions from all threads.

In next runs, after fi-inscount.txt has been created, the FI library will perform fault injection. For our implementation, the library expects a `fi-target.txt` file which contains the thread and target instruction to inject to. 
The library reads this file and randomly selects the operand and bit to flip. See the script in `<repo>/ipdps19/scripts/faultinject.py` for how we generate a set of FI targets.

Lastly, the FI library saves a log of fault injection in the file `fi-inject.txt` that contains the following information:
1. `thread`, the thread identifier the fault was injected to
2. `fi_index`, the index of the dynamic instruction
3. `op`, the index of the operand
4. `size`, the size in bytes of the operand
5. `bitflip`, the position of the flipped
If `fi-inject.txt` exists, the library will inject the fault at the same instruction, operand, and bit position specified by this file.

### Build the PINFI tool

1. Download and install the latest Intel PIN framework (https://software.intel.com/en-us/articles/pin-a-binary-instrumentation-tool-downloads)
2. Copy the directory `pinfi` in the installed PIN path under `<PIN_PATH>/source/tools`
3. Change to the copied pinfi directory `cd <PIN_PATH>/source/tools/pinfi`
4. Run `make` to build the tool

Note, the PINFI tool is configurable to select whether to inject errors in
source registers, destination registers or destination memory operands of
instructions. This is possible by editing the file `utils.h` within the `pinfi`
directory and including or excluding the preprocessor directives `FI_SRC_REG`,
`FI_DST_REG`, `FI_DST_MEM`. Those directives control whether FI is enabled for
their respective, self-descriptive targets. The PINFI fault injection
implementation follows too single fault model.

#### Use the PINFI tool

Similar to SAFIRE, PINFI must have a dynamic target instruction count before
performing fault injection. For the boostrap run, PINFI implements an `instcount` 
tool to run before executing PINFI's `faultinjection` tool. 

For example:
```
$PIN_PATH/pin -t $PIN_PATH/source/tools/pinfi/obj-intel64/instcount -- ./program <args>
```
This will run the dynamic instruction counter and generate the `pin.instcount.txt` file that contains the number of dynamic target instructions. 

PINFI's fault injection tool reads this file to perform random fault injection. Running the fault injection tool:
```
$PIN_PATH/pin -t $PIN_PATH/source/tools/pinfi/obj-intel64/faultinjection -- ./program <args>
```

The PINFI tool produces a log of fault injection in the file `pin.injection.txt` which contains the following entries:
1. `thread`, the thread identifier to which the fault was injected
2. `fi_index`, the index of the dynamic instruction
3. `reg`, the symbolic name of the operand
4. `bitflip`, the position of the flipped bit
5. `addr`, the instruction pointer address

## Contributing
To contribute to SAFIRE please send a
[pull request](https://help.github.com/articles/using-pull-requests/) on 
the master branch of this repo.

## Authors

SAFIRE was created by Giorgis Georgakoudis, georgakoudis1@llnl.gov, under
technical guidance of Ignacio Laguna (LLNL) and Hans Vandierendonck (QUB), and
design mentoring of Dimitrios S. Nikolopoulos (VT) and Martin Schulz (TUM)

### Citing SAFIRE

Please cite the following paper: 

* G. Georgakoudis, I. Laguna, H. Vandierendonck,
D. S. Nikolopoulos and M. Schulz, 
[SAFIRE: Scalable and Accurate Fault Injection for Parallel Multithreaded Applications](https://ieeexplore.ieee.org/document/8820954)
, 2019 IEEE International
Parallel and Distributed Processing Symposium (IPDPS), Rio de Janeiro, Brazil,
2019, pp. 890-899.

## License

SAFIRE is distributed under the terms of the Apache License (Version 2.0) with LLVM exceptions. Other software that is part of this repository may be under a different license, documented by the file LICENSE in its sub-directory.

All new contributions to SAFIRE must be under the Apache License (Version 2.0) with LLVM exceptions.

See files [LICENSE](LICENSE) and [NOTICE](NOTICE) for more information.

SPDX License Identifier: "Apache-2.0 WITH LLVM-exception" 

LLNL-CODE-795317


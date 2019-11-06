#ifndef FAULT_INJECTION_H
#define FAULT_INJECTION_H

#include <map>
#include "pin.H"
#include "stdio.h"
#include "stdlib.h"
#include <iostream>
#include <fstream>
#include <time.h>

KNOB<string> injection_file(KNOB_MODE_WRITEONCE, "pintool",
    "fi-output", "pin.injection.txt", "shows details of the fault injection");

KNOB<string> target_file(KNOB_MODE_WRITEONCE, "pintool",
    "fi-target", "pin.target.txt", "shows the target instruction for fault injection");

KNOB<bool> detach_knob(KNOB_MODE_WRITEONCE, "pintool",
    "detach", "0", "detach once FI is done (expermental, unstable" );

// TODO: Fix the knob to select the FI instruction types
//KNOB<string> fioption(KNOB_MODE_WRITEONCE, "pintool", "fioption", "", "specify fault injection option: all, sp, fp, ccs");

// XXX: MAX_OPS should be always greater than the maximum nuber of implicit and explicit operands
#define MAX_OPS 32

using namespace std;

std::fstream fi_output_fstream;

// Reproduce FI variables
static UINT64 fi_instr_index = 0;
static UINT64 fi_index = 0;
static UINT32 fi_bit_flip = 0;
static UINT32 fi_op = 0;

static enum {
    DO_RANDOM,
    DO_REPRODUCE,
} action;

#endif

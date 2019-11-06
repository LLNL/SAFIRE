#include<iostream>
#include<fstream>

#include <set>
#include <map>
#include <string>

#include <assert.h>

#include "pin.H"
#include "utils.h"
#include "instselector.h"

KNOB<string> instcount_file(KNOB_MODE_WRITEONCE, "pintool",
    "o", "pin.instcount.txt", "specify instruction count file name");

KNOB<bool> save_instr(KNOB_MODE_WRITEONCE, "pintool",
    "save-instr", "0", "shows details of the instruction instrumentation");

KNOB<string> instrument_file(KNOB_MODE_WRITEONCE, "pintool",
    "instr-file", "pin.instrument.txt", "shows details of the instruction instrumentation");

static UINT64 instr_iterator = 0;
static UINT64 fi_iterator = 0;

std::fstream instrument_ofstream;

//VOID countInst(UINT32 c)
VOID countInst(VOID *ip, UINT64 idx, UINT32 op, REG reg, CONTEXT *ctx)
{
  fi_iterator++;
}

// Pin calls this function every time a new instruction is encountered
VOID CountInst(TRACE trace, VOID *v)
{
  if(isValidTrace(trace))
    for( BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl) )
      for(INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins) )
        if(isValidInst(ins)) {

            instr_iterator++;

          if(save_instr.Value())
            instrument_ofstream << "img=" << IMG_Name(SEC_Img(RTN_Sec(INS_Rtn(ins)))) << ", addr=" << hexstr(INS_Address(ins))
              << ", instr=\"" << INS_Disassemble(ins) << "\"" << std::endl;

          //INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)countInst, IARG_UINT32, 1, IARG_END);
          REGSET regsIn, regsOut;
          REGSET_Clear(regsIn);
          REGSET_Clear(regsOut);

          INS_InsertCall(ins, IPOINT_AFTER, AFUNPTR(countInst),
                  IARG_THREAD_ID,
                  IARG_ADDRINT, INS_Address(ins),
                  IARG_UINT64, instr_iterator,
                  IARG_UINT32, 0,
                  IARG_UINT32, 0,
                  IARG_PARTIAL_CONTEXT, &regsIn, &regsOut,
                  IARG_END);

        }
}

VOID Fini(INT32 code, VOID *v)
{
  // Write to a file since cout and cerr maybe closed by the application
  ofstream OutFile;
  OutFile.open(instcount_file.Value().c_str());
  OutFile.setf(ios::showbase);
  OutFile << "fi_index=" << fi_iterator << endl;

  OutFile.close();

  if(save_instr.Value())
    instrument_ofstream.close();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
  cerr << "This tool counts the number of dynamic instructions executed" << endl;
  cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
  return -1;
}

int main(int argc, char * argv[])
{
  PIN_InitSymbols();
  // Initialize pin
  if (PIN_Init(argc, argv)) return Usage();

  //configInstSelector();

  if(save_instr.Value()) {
    instrument_ofstream.open(instrument_file.Value().c_str(), std::fstream::out);
    assert(instrument_ofstream.is_open() && "Cannot open instrumentation output file\n");
  }

  // Register CountInst to be called to instrument instructions in a trace
  TRACE_AddInstrumentFunction(CountInst, 0);

  // Register Fini to be called when the application exits
  PIN_AddFiniFunction(Fini, 0);

  // Start the program, never returns
  PIN_StartProgram();

  return 0;
}

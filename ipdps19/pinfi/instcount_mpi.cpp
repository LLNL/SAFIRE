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

KNOB<string> instrument_file(KNOB_MODE_WRITEONCE, "pintool",
    "fi-instr", "pin.instrument.txt", "shows details of the instruction instrumentation");

static UINT64 fi_inst = 0;
static int rank = -1;

std::fstream instrument_ofstream;

VOID countInst(UINT32 c)
{
  fi_inst += c;
}

// Pin calls this function every time a new instruction is encountered
VOID CountInst(TRACE trace, VOID *v)
{
  for( BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl) ) {
    int static_count = 0;
    // Insert a call to countInst before every bbl, passing the number of target FI instructions
    for(INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins) )
      if(isValidInst(ins)) {
        static_count++;

        instrument_ofstream << "addr=" << hexstr(INS_Address(ins)) << ", instr=\"" << INS_Disassemble(ins) << "\"" << std::endl;

	/*INS_InsertPredicatedCall(
	    ins, IPOINT_BEFORE, (AFUNPTR)countInst,
	    IARG_UINT32, 1,
	    IARG_END);*/
      }

    if(static_count > 0) {
      BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)countInst, IARG_UINT32, static_count, IARG_END);
      //BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)countInst, IARG_UINT32, static_count, IARG_ADDRINT, BBL_Address(bbl), IARG_ADDRINT, ( BBL_Address(bbl) + BBL_Size(bbl) ), IARG_END);
      //cerr << "Found BBL at " << std::hex << "0x" << BBL_Address(bbl) << " - " << "0x" << ( BBL_Address(bbl) + BBL_Size(bbl) ) << endl;
    }
  }
}

VOID Fini(INT32 code, VOID *v)
{
  // Write to a file since cout and cerr maybe closed by the application
  ofstream OutFile;
  stringstream ss;
  ss << rank << "." << instcount_file.Value();
  OutFile.open(ss.str().c_str());
  OutFile.setf(ios::showbase);
  OutFile << fi_inst << endl;

  OutFile.close();
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

  configInstSelector();

  char *rank_str = getenv("OMPI_COMM_WORLD_RANK");
  assert(rank_str != NULL && "rank_str = NULL, cannot read env variable for MPI rank\n");
  rank = atoi(rank_str);
  cout << "rank: " << rank << endl; //ggout

  stringstream ss;
  ss << rank << "." << instrument_file.Value();
  instrument_ofstream.open(ss.str().c_str(), std::fstream::out);
  assert(instrument_ofstream.is_open() && "Cannot open instrumentation output file\n");

  // Register Instruction to be called to instrument instructions
  TRACE_AddInstrumentFunction(CountInst, 0);

  // Register Fini to be called when the application exits
  PIN_AddFiniFunction(Fini, 0);

  // Start the program, never returns
  PIN_StartProgram();

  return 0;
}

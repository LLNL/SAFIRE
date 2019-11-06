#include "utils.h"
#include <iostream>

KNOB<string> instrument_libs(KNOB_MODE_WRITEONCE, "pintool",
    "instr-libs", "", "comma-separated list of libraries for instruction instrumentation");

KNOB<bool> instrument_nomain(KNOB_MODE_WRITEONCE, "pintool",
    "nomain", "0", "disable the instrumentation of the main executiable");

bool isValidTrace(TRACE trace) {
  /**
   * IMPORTANT: This is to make sure fault injections are done at the .text 
   * of the compiled code, instead of at libraries or .init/.fini sections
   */
  RTN Rtn = TRACE_Rtn(trace);
  //LOG("Exe " + RTN_Name(Rtn) + "\n");
  if (!RTN_Valid(Rtn)) { // some library instructions do not have rtn !?
    return false;
  }

  SEC Sec = RTN_Sec(Rtn);
  if(!SEC_Valid(Sec)) {
    return false;
  }

  IMG Img = SEC_Img(Sec);
  if(!IMG_Valid(Img)) {
    return false;
  }

  if(instrument_nomain && IMG_IsMainExecutable(Img))
    return false;

  if (!IMG_IsMainExecutable(Img)) {
    std::string libname = IMG_Name(Img).substr(IMG_Name(Img).find_last_of("\\/")+1);
    libname = libname.substr(0, libname.find_first_of("."));
    //LOG("Libraries " + libname + " vs " + instrument_libs.Value() + "\n");
    if(instrument_libs.Value().find(libname) == std::string::npos){
      return false;
    }
  }

  // XXX: Look for 'text' in section name. Linux: .text, Mac: __text
  if(SEC_Name(Sec).find("text") == std::string::npos) {
    //LOG("Section: " + SEC_Name(Sec) + "\n");
    return false;
  }

  std::string rtnname = RTN_Name(Rtn);
  if (rtnname.find("__libc") == 0 || rtnname.find("_start") == 0 ||
      rtnname.find("call_gmon_start") == 0 || rtnname.find("frame_dummy") == 0 ||
      rtnname.find("__do_global") == 0 || rtnname.find("__stat") == 0 || 
      rtnname.find("register_tm_clones") == 0 || rtnname.find("deregister_tm_clones") == 0) {
    return false;
  }

  return true;
}

bool isValidInst(INS ins) {
  int FI_Ops = 0;
#ifdef FI_SRC_REG
  int numR = INS_MaxNumRRegs(ins);
  FI_Ops += numR;
#endif
#ifdef FI_DST_REG
  int numW = INS_MaxNumWRegs(ins);
  for(int i = 0; i<numW; i++) {
      // Skip IP dst: cannot modify directly IP
      // TODO: Instead, modifying IP could be achieved by using a JMP instruction
      if(!REGx_IsInstrPtr(INS_RegW(ins, i)))
          FI_Ops++;
  }
#endif
#ifdef FI_DST_MEM
  // TODO: Think about scatter/gather operations, ignore for now
  if(INS_IsMemoryWrite(ins) && INS_hasKnownMemorySize(ins)) {
      // XXX: Can inject only to instruction with known control-flow
      if(INS_Next(ins) != INS_Invalid())
          FI_Ops++;
  }
#endif

  if(!FI_Ops) {
      //LOG("FIOps = 0!\n");
      return false;
  }

  // Cannot inject after these instructions
  if(INSx_MayChangeControlFlow(ins))
      return false;

  return true;
}

bool REGx_IsInstrPtr(REG reg) {
    if(reg == REG_RIP || reg == REG_EIP || reg == REG_IP)
        return true;

    return false;
}

bool INSx_MayChangeControlFlow(INS ins) {
    if(!INS_HasFallThrough(ins))
        return true;

    for(UINT32 i = 0; i < INS_MaxNumWRegs(ins); i++) {
        REG reg = INS_RegW(ins, i);
        if(REGx_IsInstrPtr(reg))
            return true;
    }

    return false;
}

bool REGx_IsStackPtr(REG reg) {
    if(reg == REG_RSP || reg == REG_ESP || reg == REG_SP)
        return true;

    return false;
}

bool is_frameptrReg(REG reg){
    if(reg == REG_RBP || reg == REG_EBP || reg == REG_BP)
        return true;
    return false;
}

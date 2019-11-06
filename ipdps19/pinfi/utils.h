#ifndef UTIL_H
#define UTIL_H

#include "pin.H"

// TODO: Convert to KNOBs
//#define FI_SRC_REG
#define FI_DST_REG
//#define FI_DST_MEM


enum OP_TYPE { REG_SRC, REG_DST, MEM_DST };

bool isValidTrace(TRACE trace);
bool isValidInst(INS ins);
bool REGx_IsInstrPtr(REG reg);
bool INSx_MayChangeControlFlow(INS ins);
bool REGx_IsStackPtr(REG reg);

#endif

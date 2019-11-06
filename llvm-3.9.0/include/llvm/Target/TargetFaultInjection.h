//===-- llvm/Target/TargetInstrInfo.h - Instruction Info --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// ggeorgak, 01/23/17
//
// This file describes the target machine 
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TARGET_TARGETFAULTINJECTION_H
#define LLVM_TARGET_TARGETFAULTINJECTION_H

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/Support/RandomNumberGenerator.h"

namespace llvm {
    class TargetFaultInjection {
        public:
            TargetFaultInjection();
            virtual ~TargetFaultInjection();
            virtual void injectFault(MachineFunction &MF,
                    MachineInstr &MI,
                    std::vector<MCPhysReg> const &FIRegs,
                    MachineBasicBlock &InstSelMBB,
                    MachineBasicBlock &PreFIMBB,
                    SmallVector<MachineBasicBlock *, 4> &OpSelMBBs,
                    SmallVector<MachineBasicBlock *, 4> &FIMBBs,
                    MachineBasicBlock &PostFIMBB) const = 0;
            virtual void injectMachineBasicBlock(MachineBasicBlock &SelMBB,
                    MachineBasicBlock &JmpDetachMBB,
                    MachineBasicBlock &JmpFIMBB,
                    MachineBasicBlock &MBB,
                    MachineBasicBlock &CopyMBB,
                    uint64_t TargetInstrCount) const = 0;
    };
} // end namespace llvm

#endif


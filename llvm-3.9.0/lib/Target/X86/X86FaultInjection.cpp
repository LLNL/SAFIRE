#include "X86InstrInfo.h"
#include "X86Subtarget.h"
#include "X86FaultInjection.h"
#include "X86MachineFunctionInfo.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "X86InstrBuilder.h"

#include "llvm/CodeGen/LivePhysRegs.h"

using namespace llvm;

/* TODO: 
 * 1. Optimize the FI process in general
 *    a. Don't spill after InstSelMBB, FI in context
 *    b. Reduce alignment depending on spilled regs
 */

// XXX: slowdown for storing string
//#define INSTR_PRINT

// Offset globals
int RSPOffset = 0, RBPOffset = -8, RAXOffset = -16;
int StackOffset = 0;

int64_t emitAllocateStackAlign(MachineBasicBlock &MBB, MachineBasicBlock::iterator I, int64_t size, int64_t Alignment)
{
    MachineFunction &MF = *MBB.getParent();
    const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();

    int64_t Offset = -StackOffset;
    Offset += size;

    int64_t AlignedStackSize = size + ( (Offset%Alignment) > 0 ? (Alignment - (Offset%Alignment)) : 0 );

    addRegOffset(BuildMI(MBB, I, DebugLoc(), TII.get(X86::LEA64r), X86::RSP), X86::RSP, false, -AlignedStackSize);

    StackOffset -= AlignedStackSize;
    //dbgs() << "emitAllocStack StackOffset: " << StackOffset << "\n"; //DBG_SAFIRE

    return AlignedStackSize;
}


void emitDeallocateStack(MachineBasicBlock &MBB, MachineBasicBlock::iterator I, int64_t size)
{
    MachineFunction &MF = *MBB.getParent();
    const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();

    addRegOffset(BuildMI(MBB, I, DebugLoc(), TII.get(X86::LEA64r), X86::RSP), X86::RSP, false, size);

    StackOffset += size;
    //dbgs() << "emitFreeStack StackOffset: " << StackOffset << "\n"; //DBG_SAFIRE
}


// XXX: emitPushReg and others assume starting from a 16B aligned stack
void emitPushReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator I, const MCPhysReg Reg)
{
    MachineFunction &MF = *MBB.getParent();
    const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();

    BuildMI(MBB, I, DebugLoc(), TII.get(X86::PUSH64r)).addReg(Reg);
    StackOffset -= 8;
    //dbgs() << "emitPushReg StackOffset: " << StackOffset << "\n"; //DBG_SAFIRE
}

void emitPopReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator I, const MCPhysReg Reg)
{
    MachineFunction &MF = *MBB.getParent();
    const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();

    BuildMI(MBB, I, DebugLoc(), TII.get(X86::POP64r)).addReg(Reg);
    StackOffset += 8;
    //dbgs() << "emitPushReg StackOffset: " << StackOffset << "\n"; //DBG_SAFIRE
}

void emitPushContextRegList(std::vector<MCPhysReg> &saveRegs, MachineBasicBlock &MBB, MachineBasicBlock::iterator I, int64_t Alignment)
{
    //XXX: Assumes stack has been pre-aligned

    // XXX: Push context, dynamic linker doesn't preserve volatile regs
    MachineFunction &MF = *MBB.getParent();
    const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
    const MachineRegisterInfo &MRI = MF.getRegInfo();
    const TargetRegisterInfo &TRI = *MRI.getTargetRegisterInfo();

    int64_t RegStackSize = 0; 

    // compute size for >64-bit regs
    for(MCPhysReg Reg : saveRegs) {
        if( !X86::GR64RegClass.contains(Reg) ) {
            const TargetRegisterClass *TRC = TRI.getMinimalPhysRegClass(Reg);
            switch( TRC->getSize() ) {
                case 16:
                    RegStackSize += 16;
                    break;
                case 32:
                    RegStackSize += 32;
                    break;
                case 64:
                    RegStackSize += 64;
                    //dbgs() << PrintReg(Reg, &TRI) << "\n";
                    //assert(false && "Unsupported 512-bit register");
                    break;
            }
        }
    }

    if(RegStackSize) {
        emitAllocateStackAlign( MBB, I, RegStackSize, Alignment);

        int RegStackOffset = 0;
        // emit store instructions
        for(MCPhysReg Reg : saveRegs) {
            if( ! X86::GR64RegClass.contains(Reg) ) {
                const TargetRegisterClass *TRC = TRI.getMinimalPhysRegClass(Reg);
                switch( TRC->getSize() ) {
                    case 16:
                        addRegOffset(BuildMI(MBB, I, DebugLoc(), TII.get( X86::MOVAPSmr )), X86::RSP, false, RegStackOffset).addReg(Reg);
                        RegStackOffset += 16;
                        break;
                    case 32:
                        addRegOffset(BuildMI(MBB, I, DebugLoc(), TII.get(X86::VMOVAPSYmr)), X86::RSP, false, RegStackOffset).addReg(Reg);
                        RegStackOffset += 32;
                        break;
                    case 64:
                        addRegOffset(BuildMI(MBB, I, DebugLoc(), TII.get(X86::VMOVAPSZmr)), X86::RSP, false, RegStackOffset).addReg(Reg);
                        RegStackOffset += 64;
                        break;
                        //return load ? X86::VMOVAPSZrm : X86::VMOVAPSZmr;
                        //dbgs() << PrintReg(Reg, &TRI) << "\n";
                        //assert(false && "Unsupported 512-bit register");
                }
            }
        }
    }

    // store GR64 registers
    for(MCPhysReg Reg : saveRegs) {
        if( X86::GR64RegClass.contains(Reg) ) {
            BuildMI(MBB, I, DebugLoc(), TII.get(X86::PUSH64r)).addReg(Reg);
            StackOffset -= 8;
        }
    }
    //dbgs() << "emitPushCtx StackOffset: " << StackOffset << "\n"; //DBG_SAFIRE
}
void emitPopContextRegList(std::vector<MCPhysReg> &saveRegs, MachineBasicBlock &MBB, MachineBasicBlock::iterator I, int Alignment)
{
    MachineFunction &MF = *MBB.getParent();
    const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
    const MachineRegisterInfo &MRI = MF.getRegInfo();
    const TargetRegisterInfo &TRI = *MRI.getTargetRegisterInfo();

    // load GR64 registers -- note reverse order
    for( auto it = saveRegs.rbegin(); it != saveRegs.rend(); it++ ) {
        MCPhysReg Reg = *it;
        if( X86::GR64RegClass.contains(Reg) ) {
            BuildMI(MBB, I, DebugLoc(), TII.get(X86::POP64r)).addReg(Reg);
            StackOffset += 8;
        }
    }

    int64_t RegStackSize = 0; 

    // compute size for >64-bit regs -- order doesn't matter
    for(MCPhysReg Reg : saveRegs) {
        if( !X86::GR64RegClass.contains(Reg) ) {
            const TargetRegisterClass *TRC = TRI.getMinimalPhysRegClass(Reg);
            switch( TRC->getSize() ) {
                case 16:
                    RegStackSize += 16;
                    break;
                case 32:
                    RegStackSize += 32;
                    break;
                case 64:
                    RegStackSize += 64;
                    break;
                    //dbgs() << PrintReg(Reg, &TRI) << "\n";
                    //assert(false && "Unsupported 512-bit register");
            }
        }
    }

    if(RegStackSize) {
        int RegStackOffset = 0;
        // emit load instructions -- note original order, restoring with MOV does not change the stack
        for( MCPhysReg Reg : saveRegs ) {
            if( ! X86::GR64RegClass.contains(Reg) ) {
                const TargetRegisterClass *TRC = TRI.getMinimalPhysRegClass(Reg);
                switch( TRC->getSize() ) {
                    case 16:
                        addRegOffset(BuildMI(MBB, I, DebugLoc(), TII.get( X86::MOVAPSrm ), Reg), X86::RSP, false, RegStackOffset);
                        RegStackOffset += 16;
                        break;
                    case 32:
                        addRegOffset(BuildMI(MBB, I, DebugLoc(), TII.get( X86::VMOVAPSYrm ), Reg), X86::RSP, false, RegStackOffset);
                        RegStackOffset += 32;
                        break;
                    case 64:
                        addRegOffset(BuildMI(MBB, I, DebugLoc(), TII.get( X86::VMOVAPSZrm ), Reg), X86::RSP, false, RegStackOffset);
                        RegStackOffset += 64;
                        break;
                        //return load ? X86::VMOVAPSZrm : X86::VMOVAPSZmr;
                        //dbgs() << PrintReg(Reg, &TRI) << "\n";
                        //assert(false && "Unsupported 512-bit register");
                }
            }
        }

        // Align RegStackSize before subtracting
        RegStackSize += ( (RegStackSize%Alignment) > 0 ? (Alignment - (RegStackSize%Alignment)) : 0 );
        emitDeallocateStack( MBB, I, RegStackSize );
    }

    //dbgs() << "emitPushCtx StackOffset: " << StackOffset << "\n"; //DBG_SAFIRE
}

void emitAlignStack(MachineBasicBlock &MBB, MachineBasicBlock::iterator I, int64_t Alignment)
{
    // Save frame registers (RSP, RBP, RAX), use RBP for addressing RSPOffset, RBPOffset, RAXOffset
    MachineFunction &MF = *MBB.getParent();
    const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();

    BuildMI(MBB, I, DebugLoc(), TII.get(X86::AND64ri8), X86::RSP).addReg(X86::RSP).addImm(-Alignment);
}

void emitSaveFrameFlags(MachineBasicBlock &MBB, MachineBasicBlock::iterator I)
{
    MachineFunction &MF = *MBB.getParent();
    const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
    X86MachineFunctionInfo *X86MFI = MF.getInfo<X86MachineFunctionInfo>();

    // Stay clear of the red zone, 128 bytes
    // XXX: This MUST be done even FI in RSP, it will be adjusted anyway at PostFIMBB
    if(X86MFI->getUsesRedZone())
        // LEA to adjust RSP (does not clobber flags)
        addRegOffset(BuildMI(MBB, I, DebugLoc(), TII.get(X86::LEA64r), X86::RSP), X86::RSP, false, -128);

    // PUSH RSP
    BuildMI(MBB, I, DebugLoc(), TII.get(X86::PUSH64r)).addReg(X86::RSP);
    // PUSH RBP
    BuildMI(MBB, I, DebugLoc(), TII.get(X86::PUSH64r)).addReg(X86::RBP);
    // RBP <- original RSP 
    addRegOffset(BuildMI(MBB, I, DebugLoc(), TII.get(X86::LEA64r), X86::RBP), X86::RSP, false, -RBPOffset);

    // XXX: We have to store EFLAGS, they may be live during the execution of this BB
    // PUSH RAX used for saving flags, required by LAHF/SAHF instructions
    BuildMI(MBB, I, DebugLoc(), TII.get(X86::PUSH64r)).addReg(X86::RAX);
    // STORE flags
    BuildMI(MBB, I, DebugLoc(), TII.get(X86::SETOr), X86::AL);
    BuildMI(MBB, I, DebugLoc(), TII.get(X86::LAHF));
}

void emitRestoreFrameFlags(MachineBasicBlock &MBB, MachineBasicBlock::iterator I)
{
    MachineFunction &MF = *MBB.getParent();
    const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
    X86MachineFunctionInfo *X86MFI = MF.getInfo<X86MachineFunctionInfo>();

    // Restore EFLAGS
    BuildMI(MBB, I, DebugLoc(), TII.get(X86::ADD8ri), X86::AL).addReg(X86::AL).addImm(INT8_MAX);
    BuildMI(MBB, I, DebugLoc(), TII.get(X86::SAHF));
    addRegOffset(BuildMI(MBB, I, DebugLoc(), TII.get(X86::MOV64rm), X86::RAX), X86::RBP, false, RAXOffset);
    addRegOffset(BuildMI(MBB, I, DebugLoc(), TII.get(X86::MOV64rm), X86::RSP), X86::RBP, false, RSPOffset);
    // Restore RBP last 
    addRegOffset(BuildMI(MBB, I, DebugLoc(), TII.get(X86::MOV64rm), X86::RBP), X86::RBP, false, RBPOffset);
    if(X86MFI->getUsesRedZone())
        // LEA adjust SP
        addRegOffset(BuildMI(MBB, I, DebugLoc(), TII.get(X86::LEA64r), X86::RSP), X86::RSP, false, 128);
}

// Fill saveRegs with LiveRegs
void fillSaveRegs(std::vector<MCPhysReg> &saveRegs, LivePhysRegs &LiveRegs, const TargetRegisterInfo *TRI)
{
    //dbgs() << "==== LIVEREGS ===\n";
    //LiveRegs.dump();
    // Remove subregs
    for(MCPhysReg Reg : LiveRegs) {
        bool ContainsSuperReg = false;
        for (MCSuperRegIterator SReg(Reg, TRI); SReg.isValid(); ++SReg) {
            if (LiveRegs.contains(*SReg)) {
                ContainsSuperReg = true;
                break;
            }
        }
        if (ContainsSuperReg)
            continue;

        MCPhysReg SReg64 = Reg;
        for (MCSuperRegIterator SReg(Reg, TRI, true); SReg.isValid(); ++SReg) {
            if( X86::GR64RegClass.contains(*SReg) ) {
                SReg64 = *SReg;
                break;
            }
        }
        //dbgs() << "Reg " << PrintReg(Reg, &TRI) << " -> " << PrintReg(SReg64, &TRI) << "\n"; //DBG_SAFIRE

        // Skip registers that we save already or don't need saving (CSR registers)
        if(! (SReg64 == X86::RIP || SReg64 == X86::RSP || SReg64 == X86::RBP || SReg64 == X86::RAX || SReg64 == X86::EFLAGS ||
                    SReg64 == X86::RBX || SReg64 == X86::R12 || SReg64 == X86::R13 || SReg64 == X86::R14 || SReg64 == X86::R15 ) )
            if( std::find(saveRegs.begin(), saveRegs.end(), SReg64) == saveRegs.end() )
                saveRegs.push_back(SReg64);
    }

    // XXX: store from larger size to smaller to avoid re-aligning the stack
    std::sort( saveRegs.begin(), saveRegs.end(), 
            [TRI](MCPhysReg a, MCPhysReg b) { 
            const TargetRegisterClass *TRC_a = TRI->getMinimalPhysRegClass(a);
            const TargetRegisterClass *TRC_b = TRI->getMinimalPhysRegClass(b);
            return (TRC_a->getSize() > TRC_b->getSize());
            } );
    /*dbgs() << "RegList:  ";
    for (auto i: saveRegs)
        dbgs() << PrintReg(i, TRI) << ' ';
    dbgs() << "\n";
    dbgs() << "==== END LIVEREGS ===\n";*/
}

void X86FaultInjection::injectMachineBasicBlock(
        MachineBasicBlock &SelMBB,
        MachineBasicBlock &JmpDetachMBB,
        MachineBasicBlock &JmpFIMBB,
        MachineBasicBlock &OriginalMBB,
        MachineBasicBlock &CopyMBB,
        uint64_t TargetInstrCount) const
{
    MachineFunction &MF = *SelMBB.getParent();
    const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
    const X86Subtarget &Subtarget = MF.getSubtarget<X86Subtarget>();
    const MachineRegisterInfo &MRI = MF.getRegInfo();
    const TargetRegisterInfo &TRI = *MRI.getTargetRegisterInfo();

    std::vector<MCPhysReg> saveRegs;

    LivePhysRegs LiveRegs;
    LiveRegs.init(&TRI);
    LiveRegs.clear();
    // Add LiveIns, we're instrumenting the start of the BB
    // XXX: Re-using liveins from the instrumented MBB
    LiveRegs.addLiveIns(SelMBB);

    // Add used registers
    saveRegs.push_back(X86::RAX);
    saveRegs.push_back(X86::RDI);
    saveRegs.push_back(X86::RSI);
    //dbgs() << "==== SELMBB ====\n";
    //SelMBB.dump();
    fillSaveRegs(saveRegs, LiveRegs, &TRI);
    //dbgs() << "==== END SELMBB ====\n";

    /* ============================================================= CREATE SelMBB ========================================================== */

    {
        emitSaveFrameFlags( SelMBB, SelMBB.end() );
    }

    // XXX: Align the stack on a 64-byte boundary. We don't know alignment on entry,
    // hence we force it to the largest register. 
    // This solves three problems:
    // 1. Calling a function needs 16B alignment, 64B covers that
    // 2. Vector registers when pushing the context, largest are ZMM 64B
    // 3. PXOR for FI on vector registers (largest ZMM) is faster 
    // The original RSP is saved in RBP and restored in emitRestoreFrameFlags
    {
        emitAlignStack( SelMBB, SelMBB.end(), 64);
    }

    {
        emitPushContextRegList( saveRegs, SelMBB, SelMBB.end(), 64 );

        // MOV RSI <= MBB.size(), selMBB arg2 (uint64_t, number of instructions)
        BuildMI(SelMBB, SelMBB.end(), DebugLoc(), TII.get(X86::MOV64ri), X86::RSI).addImm(TargetInstrCount);
        // Allocate stack space for out arg1
        int64_t AlignedStackSize = emitAllocateStackAlign(SelMBB, SelMBB.end(), 8, 64 );
        //dbgs() << "AlignedStackSize:" << AlignedStackSize << "\n"; //DBG_SAFIRE
        // MOV RDI <= RSP, selMBB arg1
        addRegOffset(BuildMI(SelMBB, SelMBB.end(), DebugLoc(), TII.get(X86::LEA64r), X86::RDI), X86::RSP, false, 0);
        int64_t RetOffset = StackOffset;

        // XXX: Create the external symbol and get target flags (e.g, X86II::MO_PLT) for linking
        MachineOperand MO = MachineOperand::CreateES("selMBB");
        MO.setTargetFlags( Subtarget.classifyGlobalFunctionReference( nullptr, *MF.getMMI().getModule() ) );
        BuildMI(SelMBB, SelMBB.end(), DebugLoc(), TII.get(X86::CALL64pcrel32)).addOperand( MO );

        // TEST for jump (see code later), XXX: THIS SETS FLAGS FOR THE JMP, be careful not to mess with them until the branch
        addDirectMem(BuildMI(SelMBB, SelMBB.end(), DebugLoc(), TII.get(X86::TEST8mi)), X86::RSP).addImm(0x2);

        emitDeallocateStack( SelMBB, SelMBB.end(), AlignedStackSize );

        emitPopContextRegList( saveRegs, SelMBB, SelMBB.end(), 64 );

        SmallVector<MachineOperand, 1> Cond;
        Cond.push_back(MachineOperand::CreateImm(X86::COND_NE));
        // XXX: "The CFG information in MBB.Predecessors and MBB.Successors must be valid before calling this function."
        // Successors added in MCFaultInjectionPass
        TII.InsertBranch(SelMBB, &JmpDetachMBB, &JmpFIMBB, Cond, DebugLoc());

        /*dbgs() << "SelMBB\n";
          SelMBB.dump();
          dbgs() << "====\n";
          assert(false && "CHECK!\n");*/

        // JmpDetachMBB
        {
            emitRestoreFrameFlags(JmpDetachMBB, JmpDetachMBB.end());

            /*dbgs() << "JmpDetachMBB\n";
              JmpDetachMBB.dump();
              dbgs() << "====\n";
              assert(false && "CHECK!\n");*/
        }

        // JmpFIMBB
        {
            // add test for FI
            addRegOffset(BuildMI(JmpFIMBB, JmpFIMBB.end(), DebugLoc(), TII.get(X86::TEST8mi)), X86::RSP, false, RetOffset).addImm(0x1);

            SmallVector<MachineOperand, 1> Cond;
            Cond.push_back(MachineOperand::CreateImm(X86::COND_E));
            // XXX: "The CFG information in MBB.Predecessors and MBB.Successors must be valid before calling this function."
            // Successors added in target-indep MCFaultInjectionPass
            TII.InsertBranch(JmpFIMBB, &OriginalMBB, &CopyMBB, Cond, DebugLoc());

            /*dbgs() << "JmpFIMBB\n";
              JmpFIMBB.dump();
              dbgs() << "====\n";
              assert(false && "CHECK!\n");*/
        }

        // OriginalMBB, jump from JmpFIMBB
        {
            emitRestoreFrameFlags(OriginalMBB, OriginalMBB.begin());
        }
        // CopyMBB, jump from JmpFIMBB
        {
            emitRestoreFrameFlags(CopyMBB, CopyMBB.begin());
        }
    }

    //dbgs() << "StackOffset:" << StackOffset << "\n"; //DBG_SAFIRE
    assert(StackOffset == 0 && "StackOffset must be 0\n");
    //assert(false && "CHECK!\n");
}

void X86FaultInjection::injectFault(MachineFunction &MF,
        MachineInstr &MI,
        std::vector<MCPhysReg> const &FIRegs,
        MachineBasicBlock &InstSelMBB,
        MachineBasicBlock &PreFIMBB,
        SmallVector<MachineBasicBlock *, 4> &OpSelMBBs,
        SmallVector<MachineBasicBlock *, 4> &FIMBBs,
        MachineBasicBlock &PostFIMBB) const
{
    const X86Subtarget &Subtarget = MF.getSubtarget<X86Subtarget>();
    const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
    const MachineRegisterInfo &MRI = MF.getRegInfo();
    const TargetRegisterInfo &TRI = *MRI.getTargetRegisterInfo();

    // XXX: PUSHF/POPF are broken: https://reviews.llvm.org/D6629
    //assert(Subtarget.hasLAHFSAHF() && "Unsupported Subtarget: MUST have LAHF/SAHF\n");

    LivePhysRegs LiveRegs;
    LiveRegs.init(&TRI);
    std::vector<MCPhysReg> saveRegs;
    // Add used registers
    saveRegs.push_back(X86::RAX);
    saveRegs.push_back(X86::RDI);
    saveRegs.push_back(X86::RSI);
    saveRegs.push_back(X86::RDX);
    saveRegs.push_back(X86::RCX);
    
    LiveRegs.clear();
    MachineBasicBlock *MBB = MI.getParent();
    LiveRegs.addLiveOuts(*MBB);

    // Find live registers
    for (auto I = MBB->rbegin(); (&*I) != &MI; ++I)
        LiveRegs.stepBackward(*I);

    /*dbgs() << "==== MBB ====\n";
    dbgs() << "=== MI ===\n";
    MI.dump();
    dbgs() << "=== END MI ===\n";*/
    //MBB->dump();
    fillSaveRegs(saveRegs, LiveRegs, &TRI);
    //dbgs() << "==== END MBB ====\n";
    
    unsigned MaxRegSize = 0;
    // Find maximum size of target register to allocate stack space for the bitmask
    for(auto FIReg : FIRegs) {
        const TargetRegisterClass *TRC = TRI.getMinimalPhysRegClass(FIReg);
        // Size is in bytes
        unsigned RegSize = TRC->getSize();
        MaxRegSize = RegSize > MaxRegSize ? RegSize : MaxRegSize;

        //dbgs() << "FIReg:" << FIReg << ", RegName:" << TRI.getName(FIReg) << ", RegSizeBits:" << 8*RegSize<< ", ";
    }
    //dbgs() << "\n";

    assert(MaxRegSize > 0 && "MaxRegSize must be > 0\n");

    /* ============================================================= CREATE InstSelMBB ========================================================== */

    // Save frame registers (RSP, RBP, RAX), use RBP for addressing
    {
        emitSaveFrameFlags( InstSelMBB, InstSelMBB.end() );
    }

    // XXX: Align stack on 64B, see earlier comment
    {
        emitAlignStack( InstSelMBB, InstSelMBB.end(), 64 );
    }

    {
        emitPushContextRegList( saveRegs, InstSelMBB, InstSelMBB.end(), 64 );

#ifdef INSTR_PRINT
        std::string instr_str;
        llvm::raw_string_ostream rso(instr_str);
        //MI.print(rso, true); //skip operands
        MI.print(rso); //include operands
        //dbgs() << rso.str() << "size:" << rso.str().size() << "c_str size:" << strlen(rso.str().c_str())+1 << "\n";
#endif
#ifdef INSTR_PRINT
        // out arg1 + in arg str
        int64_t size = 8 + rso.str().size()+1/*str size + 1 for NUL char*/;
#else
        // out arg1
        int64_t size = 8;
#endif
        int64_t AlignedStackSize = emitAllocateStackAlign( InstSelMBB, InstSelMBB.end(), size, 64 );
        // MOV RDI <= RSP, selInst out arg1
        addRegOffset(BuildMI(InstSelMBB, InstSelMBB.end(), DebugLoc(), TII.get(X86::LEA64r), X86::RDI), X86::RSP, false, 0);
#ifdef INSTR_PRINT
        // LEA RSI <= &op, doInject arg2 (uint64_t *, &op, 8B), 8 is the offset from arg1
        addRegOffset(BuildMI(InstSelMBB, InstSelMBB.end(), DebugLoc(), TII.get(X86::LEA64r), X86::RSI), X86::RSP, false, 8);
        int i = 0;
        for(char c : rso.str()) {
            addRegOffset(BuildMI(InstSelMBB, InstSelMBB.end(), DebugLoc(), TII.get(X86::MOV8mi)), X86::RSI, false, i*sizeof(char)).addImm(c);
            i++;
        }
        // Add terminating NUL character
        addRegOffset(BuildMI(InstSelMBB, InstSelMBB.end(), DebugLoc(), TII.get(X86::MOV8mi)), X86::RSI, false, i*sizeof(char)).addImm(0);
#endif

        // XXX: Create the external symbol and get target flags (e.g, X86II::MO_PLT) for linking
        MachineOperand MO = MachineOperand::CreateES("selInst");
        MO.setTargetFlags( Subtarget.classifyGlobalFunctionReference( nullptr, *MF.getMMI().getModule() ) );
        BuildMI(InstSelMBB, InstSelMBB.end(), DebugLoc(), TII.get(X86::CALL64pcrel32)).addOperand( MO );

        // TEST for jump (see code later), XXX: THIS SETS FLAGS FOR THE JMP, be careful not to mess with them until the branch
        addDirectMem(BuildMI(InstSelMBB, InstSelMBB.end(), DebugLoc(), TII.get(X86::TEST8mi)), X86::RSP).addImm(0x1);

        emitDeallocateStack( InstSelMBB, InstSelMBB.end(), AlignedStackSize );

        emitPopContextRegList( saveRegs, InstSelMBB, InstSelMBB.end(), 64 );

        SmallVector<MachineOperand, 1> Cond;
        Cond.push_back(MachineOperand::CreateImm(X86::COND_E));
        InstSelMBB.addSuccessor(&PostFIMBB);
        InstSelMBB.addSuccessor(&PreFIMBB);
        TII.InsertBranch(InstSelMBB, &PostFIMBB, &PreFIMBB, Cond, DebugLoc());
    }

    /* ============================================================= END OF InstSelMBB ========================================================== */

    /* ============================================================== CREATE PreFIMBB ============================================================== */

    // SystemV x64 calling conventions, args: RDI, RSI, RDX, RCX, R8, R9, XMM0-7, RTL

    emitPushContextRegList( saveRegs, PreFIMBB, PreFIMBB.end(), 64 );

    // The size and number of pointer arguments other than the bitmask
    unsigned PointerDataSize = 8;
    // SUB to create stack space for doInject arg2, arg3, arg4
    // TODO: Reduce stack space, ops, size array fit in uint16_t types
    // XXX: Align to 16-bytes
    int64_t size = (PointerDataSize + FIRegs.size() * PointerDataSize + MaxRegSize);
    int64_t AlignedStackSize = emitAllocateStackAlign( PreFIMBB, PreFIMBB.end(), size, 64 );
    // MOV RDI <= FIRegs.size(), doInject arg1 (uint64_t, number of ops)
    BuildMI(PreFIMBB, PreFIMBB.end(), DebugLoc(), TII.get(X86::MOV64ri), X86::RDI).addImm(FIRegs.size());
    // LEA RSI <= &op, doInject arg2 (uint64_t *, &op, 8B)
    addRegOffset(BuildMI(PreFIMBB, PreFIMBB.end(), DebugLoc(), TII.get(X86::LEA64r), X86::RSI), X86::RSP, false, MaxRegSize + PointerDataSize * FIRegs.size());
    // LEA RDX <= &size, doInject arg3 (uint64_t *, &size, number of ops * 8B)
    addRegOffset(BuildMI(PreFIMBB, PreFIMBB.end(), DebugLoc(), TII.get(X86::LEA64r), X86::RDX), X86::RSP, false, MaxRegSize);
    // MOV RDX <= RSP, doInject arg4 (uint8_t *, &bitmask, MaxRegSize B)
    addRegOffset(BuildMI(PreFIMBB, PreFIMBB.end(), DebugLoc(), TII.get(X86::LEA64r), X86::RCX), X86::RSP, false, 0);
    int64_t BitmaskStackOffset = StackOffset;
    // XXX: Beward of type casts, signed integers needed
    int64_t OpSelStackOffset = StackOffset + (int64_t)MaxRegSize + (int64_t)PointerDataSize * FIRegs.size();
    // Fill in size array
    for(unsigned i = 0; i < FIRegs.size(); i++) {
        unsigned FIReg = FIRegs[i];
        const TargetRegisterClass *TRC = TRI.getMinimalPhysRegClass(FIReg);
        // Size is in bytes
        unsigned RegSize = TRC->getSize();
        MaxRegSize = RegSize > MaxRegSize ? RegSize : MaxRegSize;
        addRegOffset(BuildMI(PreFIMBB, PreFIMBB.end(), DebugLoc(), TII.get(X86::MOV64mi32)), X86::RDX, false, i * PointerDataSize).addImm(RegSize);
    }

    //addDirectMem(BuildMI(PreFIMBB, PreFIMBB.end(), DebugLoc(), TII.get(X86::MOV64mi32)), X86::RDI).addImm(0x0);

    // XXX: Create the external symbol and get target flags (e.g, X86II::MO_PLT) for linking
    MachineOperand MO = MachineOperand::CreateES("doInject");
    MO.setTargetFlags( Subtarget.classifyGlobalFunctionReference( nullptr, *MF.getMMI().getModule() ) );
    BuildMI(PreFIMBB, PreFIMBB.end(), DebugLoc(), TII.get(X86::CALL64pcrel32)).addOperand( MO );

    // POP doInject arg2, arg3, ar4
    emitDeallocateStack( PreFIMBB, PreFIMBB.end(), AlignedStackSize );

    emitPopContextRegList( saveRegs, PreFIMBB, PreFIMBB.end(), 64 );

    PreFIMBB.addSuccessor(OpSelMBBs.front()); 
    TII.InsertBranch(PreFIMBB, OpSelMBBs.front(), nullptr, None, DebugLoc());

    /* ============================================================= END OF PreFIMBB ============================================================= */

    /* ============================================================== CREATE OpSelMBBs =============================================================== */

    // Jump tables to selected op
    for(int OpIdx = FIRegs.size()-1, OpSelIdx = 0; OpIdx > 0; OpIdx--, OpSelIdx++) { //no need to jump to 0th operand, fall through
        MachineBasicBlock &OpSelMBB = *OpSelMBBs[OpSelIdx];
        MachineBasicBlock *NextOpSelMBB = OpSelMBBs[OpSelIdx+1];
        addRegOffset(BuildMI(OpSelMBB, OpSelMBB.end(), DebugLoc(), TII.get(X86::CMP64mi8)), X86::RSP, false, OpSelStackOffset).addImm(OpIdx);
        SmallVector<MachineOperand, 1> Cond;
        Cond.push_back(MachineOperand::CreateImm(X86::COND_E));
        OpSelMBB.addSuccessor(FIMBBs[OpIdx]);
        OpSelMBB.addSuccessor(NextOpSelMBB);
        TII.InsertBranch(OpSelMBB, FIMBBs[OpIdx], NextOpSelMBB, Cond, DebugLoc());
    }
    // Add the fall through OpSelMBB
    OpSelMBBs.back()->addSuccessor(FIMBBs[0]);
    TII.InsertBranch(*(OpSelMBBs.back()), FIMBBs[0], nullptr, None, DebugLoc());

    /* ============================================================== END OF OpSelMBBs =============================================================== */


    /* ============================================================== CREATE FIMBBs =============================================================== */
    for(unsigned idx = 0; idx < FIRegs.size(); idx++) {
        unsigned FIReg = FIRegs[idx];
        MachineBasicBlock &FIMBB = *FIMBBs[idx];

        const TargetRegisterClass *TRC = TRI.getMinimalPhysRegClass(FIReg);
        unsigned RegSize = TRC->getSize();
        unsigned RegSizeBits = RegSize * 8;

        // ProxyFIReg defaults to the register itself. It can be set to a different 
        // registers if FIReg = FLAGS | SP | RAX. FI operates on ProxyFIReg, and it 
        // is later copied to FIReg, if needed.
        unsigned ProxyFIReg = FIReg;

        if(RegSizeBits <= 32) {
            // If it's one of the workhorse registers, used a different register as a proxy
            if(TRI.getSubRegIndex(X86::RSP, FIReg) || TRI.getSubRegIndex(X86::RBP, FIReg) || TRI.getSubRegIndex(X86::RAX, FIReg)) {
                ProxyFIReg = X86::RBX;

                unsigned RegStackOffset = 0;
                // Set the right offset in the stack
                if(TRI.getSubRegIndex(X86::RSP, FIReg))
                    RegStackOffset = RSPOffset;
                else if(TRI.getSubRegIndex(X86::RBP, FIReg))
                    RegStackOffset = RBPOffset;
                else if(TRI.getSubRegIndex(X86::RAX, FIReg))
                    RegStackOffset = RAXOffset;

                // PUSH Proxy to use for FI
                BuildMI(FIMBB, FIMBB.end(), DebugLoc(), TII.get(X86::PUSH64r)).addReg(ProxyFIReg);
                BitmaskStackOffset += 8;
                addRegOffset(BuildMI(FIMBB, FIMBB.end(), DebugLoc(), TII.get(X86::MOV64rm), ProxyFIReg), X86::RBP, false, RegStackOffset);
            }
            // RAX is already the proxy for EFLAGS 
            else if(FIReg == X86::EFLAGS)
                ProxyFIReg = X86::RAX;

            addRegOffset(BuildMI(FIMBB, FIMBB.end(), DebugLoc(), TII.get(X86::XOR32rm), ProxyFIReg).addReg(ProxyFIReg), X86::RSP, false, BitmaskStackOffset);

            if(TRI.getSubRegIndex(X86::RSP, FIReg) || TRI.getSubRegIndex(X86::RBP, FIReg) || TRI.getSubRegIndex(X86::RAX, FIReg)) {
                unsigned RegStackOffset = 0;
                // Set the right offset in the stack
                if(TRI.getSubRegIndex(X86::RSP, FIReg))
                    RegStackOffset = RSPOffset;
                else if(TRI.getSubRegIndex(X86::RBP, FIReg))
                    RegStackOffset = RBPOffset;
                else if(TRI.getSubRegIndex(X86::RAX, FIReg))
                    RegStackOffset = RAXOffset;

                // Store XOR result to stack
                addRegOffset(BuildMI(FIMBB, FIMBB.end(), DebugLoc(), TII.get(X86::MOV64mr)), X86::RBP, false, RegStackOffset).addReg(ProxyFIReg);
                // POP Proxy
                BuildMI(FIMBB, FIMBB.end(), DebugLoc(), TII.get(X86::POP64r)).addReg(ProxyFIReg);
            }
        }
        else if(RegSizeBits <= 64) {
            if(FIReg == X86::RSP || FIReg == X86::RBP || FIReg == X86::RAX) {
                ProxyFIReg = X86::RBX;

                unsigned RegStackOffset = 0;
                // Set the right offset in the stack
                if(FIReg == X86::RSP)
                    RegStackOffset = RSPOffset;
                else if(FIReg == X86::RBP)
                    RegStackOffset = RBPOffset;
                else if(FIReg == X86::RAX)
                    RegStackOffset = RAXOffset;

                // PUSH Proxy to use for FI
                BuildMI(FIMBB, FIMBB.end(), DebugLoc(), TII.get(X86::PUSH64r)).addReg(ProxyFIReg);
                BitmaskStackOffset += 8;
                addRegOffset(BuildMI(FIMBB, FIMBB.end(), DebugLoc(), TII.get(X86::MOV64rm), ProxyFIReg), X86::RBP, false, RegStackOffset);
            }

            addRegOffset(BuildMI(FIMBB, FIMBB.end(), DebugLoc(), TII.get(X86::XOR64rm), ProxyFIReg).addReg(ProxyFIReg), X86::RSP, false, BitmaskStackOffset);

            if(FIReg == X86::RSP || FIReg == X86::RBP || FIReg == X86::RAX) {
                unsigned RegStackOffset = 0;
                // Set the right offset in the stack
                if(FIReg == X86::RSP)
                    RegStackOffset = RSPOffset;
                else if(FIReg == X86::RBP)
                    RegStackOffset = RBPOffset;
                else if(FIReg == X86::RAX)
                    RegStackOffset = RAXOffset;

                // Store XOR result to stack
                addRegOffset(BuildMI(FIMBB, FIMBB.end(), DebugLoc(), TII.get(X86::MOV64mr)), X86::RBP, false, RegStackOffset).addReg(ProxyFIReg);
                // POP Proxy
                BuildMI(FIMBB, FIMBB.end(), DebugLoc(), TII.get(X86::POP64r)).addReg(ProxyFIReg);
            }
        }
        // XMM registers
        else if(RegSizeBits <= 128) {
            addRegOffset(BuildMI(FIMBB, FIMBB.end(), DebugLoc(), TII.get(X86::PXORrm), ProxyFIReg).addReg(ProxyFIReg), X86::RSP, false, BitmaskStackOffset);
            /*std::string str;
            llvm::raw_string_ostream rso(str);
            MI2->print(rso);
            dbgs() << "XMM " << PrintReg(FIReg, &TRI) << " MaxRegSize:" << MaxRegSize << " FIXOR: " << rso.str(); //DBG_SAFIRE*/
        }
        // YMM registers
        else if(RegSizeBits <= 256) {
            
            addRegOffset(BuildMI(FIMBB, FIMBB.end(), DebugLoc(), TII.get(X86::VXORPSYrm), ProxyFIReg).addReg(ProxyFIReg), X86::RSP, false, BitmaskStackOffset);
            /*std::string str;
            llvm::raw_string_ostream rso(str);
            MI2->print(rso);
            dbgs() << "YMM " << PrintReg(FIReg, &TRI) << " MaxRegSize:" << MaxRegSize << " FIXOR: " << rso.str(); //DBG_SAFIRE*/

        }
        // ZMM registers
        //TODO: CHECK!
        else if(RegSizeBits <= 512) {
            addRegOffset(BuildMI(FIMBB, FIMBB.end(), DebugLoc(), TII.get(X86::VXORPSZrm), ProxyFIReg).addReg(ProxyFIReg), X86::RSP, false, BitmaskStackOffset);
            /*std::string str;
            llvm::raw_string_ostream rso(str);
            MI2->print(rso);
            dbgs() << "ZMM " << PrintReg(FIReg, &TRI) << " MaxRegSize:" << MaxRegSize << " FIXOR: " << rso.str(); //DBG_SAFIRE*/
        }
        else
            assert(false && "RegSizeBits is invalid!\n");

        FIMBB.addSuccessor(&PostFIMBB);
        TII.InsertBranch(FIMBB, &PostFIMBB, nullptr, None, DebugLoc());
    }

    /* ============================================================== END OF FIMBB =============================================================== */

    /* ============================================================ CREATE PostFIMBB ============================================================= */

    {
        emitRestoreFrameFlags( PostFIMBB, PostFIMBB.end() );
    }


    /* ============================================================ END OF PostFIMBB ============================================================= */

    assert(StackOffset == 0 && "StackOffset must be 0");
}

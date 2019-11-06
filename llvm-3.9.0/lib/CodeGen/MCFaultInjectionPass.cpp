//===- MIRPrintingPass.cpp - Pass that prints out using the MIR format ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// ggeorgak, 12/21/16 @ LLNL
//
//===----------------------------------------------------------------------===//
//
// This file implements a pass to do fault injection in machine code
//
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Target/TargetMachine.h"

#include <llvm/CodeGen/MachineInstrBuilder.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/CodeGen/Passes.h>
#include "llvm/Pass.h"

#include "llvm/Analysis/Passes.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/CodeGen/MachineFunctionAnalysis.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/Support/RandomNumberGenerator.h"

#include <fstream>

using namespace llvm;

#define DEBUG_TYPE "mc-fi"

cl::opt<bool>
FIEnable("fi", cl::desc("Enable fault injection at the instruction level"), cl::init(false));

cl::opt<bool>
SaveInstrEnable("fi-save-instr", cl::desc("Save instrumentation log file: <module>-instrument.txt"), cl::init(false));

cl::opt<bool>
FILiveinsMBBEnable("fi-mbb-liveins", cl::desc("Enable fault injection at the livein registers of the MBB"), cl::init(false));

cl::opt<bool>
FFEnable("fi-ff", cl::desc("Enable basic block instrumentation and detaching for fast-forwarding instruction level FI"), cl::init(false));

cl::list<std::string>
FuncInclList("fi-funcs", cl::CommaSeparated, cl::desc("Fault injected functions"), cl::value_desc("foo1, foo2, foo3, ..."));

cl::list<std::string>
FuncExclList("fi-funcs-excl", cl::CommaSeparated, cl::desc("Exclude functions from fault injection"), cl::value_desc("foo1, foo2, foo3, ..."));

cl::list<std::string>
FIInstList("fi-inst", cl::CommaSeparated, cl::desc("(Architecture specific! Comma-separated list of instructions to target for fault injection"), cl::value_desc("add, sub, mul, ..."));

cl::list<std::string>
FIInstTypes("fi-inst-types", cl::CommaSeparated, cl::desc("Fault injected instruction types"), cl::value_desc("data,control,frame"));

cl::list<std::string>
FIRegTypes("fi-reg-types", cl::CommaSeparated, cl::desc("Fault injected registers"), cl::value_desc("dst, src"));

namespace {
  struct MCFaultInjectionPass : public MachineFunctionPass {
  private:
    enum InjectPoint {
      INJECT_BEFORE,
      INJECT_AFTER
    };

    uint64_t TotalInstrCount;
    uint64_t TotalTargetInstrCount;
    std::ofstream InstrumentFile;

    Module *M;
  public:
    static char ID;

    MCFaultInjectionPass() : MachineFunctionPass(ID) {
      if(FIEnable) dbgs() << "==== MCFAULTINJECTIONPASS ====\n"; //DBG_SAFIRE
      TotalInstrCount = 0; TotalTargetInstrCount = 0;
    }

    ~MCFaultInjectionPass() {
      if(FIEnable) {
        dbgs() << "END TotalInstrCount: " << TotalInstrCount << ", TotalTargetInstrCount:" << TotalTargetInstrCount << "\n";
        dbgs() << "==== END MCFAULTINJECTIONPASS ====\n"; //DBG_SAFIRE
      }
    }

    bool doInitialization(Module &M) override {
      this->M = &M;
      if(SaveInstrEnable)
        InstrumentFile.open((M.getName() + "-instrument.txt").str(), std::fstream::out);
      return false;
    }

    bool doFinalization(Module &M) override {
      //dbgs() << "MCFIPass finalize!" << "\n";
      if(SaveInstrEnable)
        InstrumentFile.close();
      return false;
    }

    void printMachineBasicBlock(MachineBasicBlock &MBB) {
      dbgs() << "MBB: " << MBB.getName() << ", " << MBB.getSymbol()->getName() << "\n";
      MBB.dump();
    }

    void printMachineFunction(MachineFunction &MF) {
      dbgs() << "MF: " << MF.getName() << "\n";
      for(auto &MBB : MF)
        printMachineBasicBlock(MBB);
    }

    void instrumentRegs(MachineInstr &MI, std::vector<MCPhysReg> const &FIRegs, InjectPoint IT) {
      MachineBasicBlock &MBB = *MI.getParent();
      MachineFunction &MF = *MBB.getParent();
      MachineBasicBlock::instr_iterator Iter = MI.getIterator();
      const TargetFaultInjection *TFI = MF.getSubtarget().getTargetFaultInjection();
      const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();

      MachineBasicBlock *InstSelMBB = MF.CreateMachineBasicBlock(nullptr);
      MachineBasicBlock *PreFIMBB = MF.CreateMachineBasicBlock(nullptr);
      SmallVector<MachineBasicBlock *, 4> OpSelMBBs;
      SmallVector<MachineBasicBlock *, 4> FIMBBs;
      for(unsigned i = 0; i < FIRegs.size(); i++) {
        OpSelMBBs.push_back(MF.CreateMachineBasicBlock(nullptr));
        FIMBBs.push_back(MF.CreateMachineBasicBlock(nullptr));
      }
      MachineBasicBlock *PostFIMBB = MF.CreateMachineBasicBlock(nullptr);

      MachineFunction::iterator MBBI = MBB.getIterator();
      MF.insert(++MBBI, InstSelMBB);

      MBBI = InstSelMBB->getIterator();
      MF.insert(++MBBI, PreFIMBB);

      MBBI = PreFIMBB->getIterator();
      for(auto OpSelMBB : OpSelMBBs) {
        MF.insert(++MBBI, OpSelMBB);
        MBBI = OpSelMBB->getIterator();
      }

      MBBI = OpSelMBBs.back()->getIterator();
      for(auto FIMBB : FIMBBs) {
        MF.insert(++MBBI, FIMBB);
        MBBI = FIMBB->getIterator();
      }

      //MBBI = FIMBB->getIterator();
      MBBI = FIMBBs.back()->getIterator();
      MF.insert(++MBBI, PostFIMBB);

      TFI->injectFault(MF, MI, FIRegs, *InstSelMBB, *PreFIMBB, OpSelMBBs, FIMBBs, *PostFIMBB);

      if(IT == INJECT_BEFORE)
        PostFIMBB->splice(PostFIMBB->end(), &MBB, Iter, MBB.end());
      else if(IT == INJECT_AFTER)
        PostFIMBB->splice(PostFIMBB->end(), &MBB, std::next(Iter), MBB.end());
      else
        assert(false && "InjectPoint is invalid!\n");

      PostFIMBB->transferSuccessors(&MBB);
      MachineBasicBlock *TBB = nullptr, *FBB = nullptr;
      SmallVector<MachineOperand, 4> Cond;

      if(!TII.analyzeBranch(*PostFIMBB, TBB, FBB, Cond))
        PostFIMBB->updateTerminator();

      MBB.addSuccessor(InstSelMBB);
      TII.InsertBranch(MBB, InstSelMBB, nullptr, None, DebugLoc());

      /*dbgs() << "============= MBB ============\n"; //DBG_SAFIRE
        printMachineBasicBlock(MBB);
        dbgs() << "============= EOM ============\n";*/
      MBB.updateTerminator();
      InstSelMBB->updateTerminator();
      PreFIMBB->updateTerminator();
      for(auto OpSelMBB : OpSelMBBs)
        OpSelMBB->updateTerminator();
      for(auto FIMBB : FIMBBs)
        FIMBB->updateTerminator();
    }

    void instrumentInstructionOperands(MachineInstr &MI, SmallVector<MachineOperand *, 4> &EligibleOps, InjectPoint IT) {
      MachineBasicBlock &MBB = *MI.getParent();
      MachineFunction &MF = *MBB.getParent();
      const MachineRegisterInfo &MRI = MF.getRegInfo();
      const TargetRegisterInfo &TRI = *MRI.getTargetRegisterInfo();

      // XXX: Inject errors only on super-registers to avoid duplicates
      EligibleOps.erase(std::remove_if(EligibleOps.begin(), EligibleOps.end(),
            [&TRI, &MRI, EligibleOps](MachineOperand *a) {
            for(auto b : EligibleOps)
            if(TRI.getSubRegIndex(b->getReg(), a->getReg())) return true;
            return false;
            }), EligibleOps.end());

      // XXX: WARNING! CAUTION! This implementation assumes FI happens *ONLY* in DST registers, 
      // thus it's always inserted after the instruction to instrument
      // TODO: SRC Registers
      //MachineOperand *MO = EligibleOps[rnd_idx];
      //injectFault(*MI, MO->getReg(), (MO->isUse()?INJECT_BEFORE:INJECT_AFTER));
      // XXX: Convert from MO vector to MCPhysReg vector. I'm keeping old code for continuity and 
      // upgradeability
      std::vector<MCPhysReg> FIRegs;
      for(auto MO : EligibleOps)
        if(MO->isDef()) {
          //dbgs() << TRI.getName(MO->getReg()) << ", ";
          FIRegs.push_back(MO->getReg());
        }

      instrumentRegs(MI, FIRegs, IT);
    }

    void instrumentLiveinsMBB(MachineFunction &MF) {
      std::vector<MachineBasicBlock *> MBBs;
      /*dbgs() << "========== INJECTION ============\n";
        printMachineFunction(MF); //DBG_SAFIRE
        dbgs() << "======= END OF INJECTION ========\n";*/

      for(auto &MBB : MF) {
        dbgs() << "MBB: " << MBB.getSymbol()->getName() << " ";
        unsigned count = 0;
        std::vector<MCPhysReg> FIRegs;
        dbgs() << "LiveIns: ";
        for(auto &Reg : MBB.liveins()) {
          const MachineRegisterInfo &MRI = MF.getRegInfo();
          const TargetRegisterInfo &TRI = *MRI.getTargetRegisterInfo();
          dbgs() << TRI.getName(Reg.PhysReg) << " ";
          FIRegs.push_back(Reg.PhysReg);
          count++;
        }

        // XXX: Some MBB have no instructionts, hence check with empty()
        if(count && !MBB.empty())
          MBBs.push_back(&MBB);

        dbgs() << "\n";
        continue;
      }

      for(auto MBB : MBBs) {
        unsigned count = 0;
        std::vector<MCPhysReg> FIRegs;
        dbgs() << "Inject: ";
        dbgs() << "MBB: " << MBB->getName() << ", " << MBB->getSymbol()->getName() << " ";
        for(auto &Reg : MBB->liveins()) {
          const MachineRegisterInfo &MRI = MF.getRegInfo();
          const TargetRegisterInfo &TRI = *MRI.getTargetRegisterInfo();
          dbgs() << TRI.getName(Reg.PhysReg) << " ";
          FIRegs.push_back(Reg.PhysReg);
          count++;
        }

        assert(count > 0 && "No liveins?");
        /*dbgs() << "===== BEGIN INST =====\n"; //DBG_SAFIRE
          MBB->instr_begin()->dump();
          dbgs() << "===== END   INST =====\n";*/
        //instrumentInstruction(*MBB->instr_begin(), FIRegs[rand], INJECT_BEFORE);
        assert(FIRegs.size() > 0 && "FI Regs are 0!\n");
        instrumentRegs(*MBB->instr_begin(), FIRegs, INJECT_BEFORE);
      }
    }

    std::tuple<uint64_t, uint64_t> findTargetInstructionsPair(
        SmallVector< std::pair< MachineInstr *, SmallVector<MachineOperand *, 4> >, 32> &vecFIInstr,
        MachineBasicBlock &MBB,
        bool doDataFI, bool doControlFI, bool doFrameFI, bool injectDstRegs, bool injectSrcRegs) {

      //const TargetInstrInfo &TII = *MBB.getParent()->getSubtarget().getInstrInfo();

      uint64_t InstrCount = 0;
      uint64_t TargetInstrCount = 0;

      for(MachineBasicBlock::instr_iterator Iter = MBB.instr_begin(); Iter != MBB.instr_end(); Iter++) {
        bool isData = false, isControl = false, isFrame = false;
        MachineInstr &MI = *Iter;

        if(!MI.isPseudo() && !MI.usesCustomInsertionHook()) {
          InstrCount++;
          //dbgs() << "==== FIND TARGET ====\n";
          //MI.dump(); //DBG_SAFIRE

          if(MI.isBranch() || MI.isCall() || MI.isReturn())
            isControl = true;
          else if(MI.getFlag(MachineInstr::FrameSetup) || MI.getFlag(MachineInstr::FrameDestroy))
            isFrame = true;
          else
            isData = true;

          if( MI.isCall() || MI.isReturn() ) {
            // TODO: Think about FI in CALL and RET. Cannot INJECT_AFTER since they change 
            // control flow.  CALL := PUSH(IP) & JMP, RET := POP(IP)
            //dbgs() << "skip call, ret change control flow, inject after\n";
            continue;
          }
          //dbgs() << (isData?"data, ":"") << (isControl?"control," :"") << (isFrame?"frame,":"") << " ";
          //dbgs() << "isMem:" << (MI.mayLoadOrStore()?"true":"false") << ", ";

          // Skip instructions based on FI inst types
          if(!doFrameFI && isFrame) {
            //dbgs() << "skip frame instr\n";
            continue;
          }

          if(!doDataFI && isData) {
            //dbgs() << "skip data instr\n";
            continue;
          }

          if(!doControlFI && isControl) {
            //dbgs() << "skip control instr\n";
            continue;
          }

          assert((isData || isFrame || isControl) && "Instruction type is invalid!\n");

          SmallVector<MachineOperand *, 4> EligibleOps;

          // Find if the instruction is eligible based on the operand selection
          for(auto MOIter = MI.operands_begin(); MOIter != MI.operands_end(); MOIter++) {
            MachineOperand &MO = *MOIter;

            if(MO.isReg() && MO.getReg()) {
              if(injectSrcRegs && MO.isUse())
                EligibleOps.push_back(&MO);
              else if(injectDstRegs && MO.isDef())
                EligibleOps.push_back(&MO);
            }
          }

          if(!EligibleOps.empty()) {
            vecFIInstr.push_back(std::make_pair(&MI, EligibleOps));
            TargetInstrCount++;
            //dbgs() << "FOUND TARGET\n";
          }
          else {
            //dbgs() << "skip no eligible ops\n";
          }
        }
      }

      assert(TargetInstrCount == vecFIInstr.size() && "TargetInstrCount != vecFIInstr.size()");
      return std::make_tuple(InstrCount, TargetInstrCount);
    }

    void instrumentInstructionsInMachineBasicBlock(
        SmallVector< std::pair< MachineInstr *, SmallVector<MachineOperand *, 4> >, 32> &vecFIInstr,
        MachineFunction &MF) {
      for(auto I : vecFIInstr) {
        MachineInstr *MI = I.first;
        SmallVector<MachineOperand *, 4> &EligibleOps = I.second;

        //dbgs() << "MBB: " << MI->getParent()->getSymbol()->getName() << ", ";
        //dbgs() << "FI-MI: ";
        //MI->dump(); //DBG_SAFIRE

        assert(!EligibleOps.empty() && "EligibleOps cannot be empty!\n");

        // XXX: only INJECT_AFTER, DST registers for now
        instrumentInstructionOperands(*MI, EligibleOps, INJECT_AFTER);
      }
    }

    bool runOnMachineFunction(MachineFunction &MF) override {

      uint64_t FuncInstrCount = 0;
      uint64_t FuncTargetInstrCount = 0;

      if(!FIEnable && !FILiveinsMBBEnable)
        return false;

      if(!FuncInclList.empty())
        if(std::find(FuncInclList.begin(), FuncInclList.end(), "*") == FuncInclList.end())
          if(std::find(FuncInclList.begin(), FuncInclList.end(), MF.getName()) == FuncInclList.end()) {
            dbgs() << "Skip:" << MF.getName() << "\n";
            return false;
          }

      if(!FuncExclList.empty()) {
        if(std::find(FuncExclList.begin(), FuncExclList.end(), "*") == FuncExclList.end()) {
          if(std::find(FuncExclList.begin(), FuncExclList.end(), MF.getName()) != FuncExclList.end()) {
            dbgs() << "Skip (EXCL):" << MF.getName() << "\n";
            return false;
          }
        }
        else {
          dbgs() << "Skip (EXCL):" << MF.getName() << "\n";
          return false;
        }
      }

      if(FILiveinsMBBEnable) {
        //dbgs() << "<<<<<<< Using function call for instrumentMBB >>>>>>>>>\n";
        instrumentLiveinsMBB(MF);
      }
      else if(FIEnable) {
        // TODO: Think whether error checking should be better, i.e., an invalid option at the
        // moment is ignored, perhaps reporting back an error is better
        bool doDataFI = false, doControlFI = false, doFrameFI = false;
        if(!FIInstTypes.empty()) {

          if(std::find(FIInstTypes.begin(), FIInstTypes.end(), "*") != FIInstTypes.end())
            doDataFI = doControlFI = doFrameFI = true;
          else {
            if(std::find(FIInstTypes.begin(), FIInstTypes.end(), "data") != FIInstTypes.end())
              doDataFI = true;

            if(std::find(FIInstTypes.begin(), FIInstTypes.end(), "control") != FIInstTypes.end())
              doControlFI = true;

            if(std::find(FIInstTypes.begin(), FIInstTypes.end(), "frame") != FIInstTypes.end())
              doFrameFI = true;
          }

          assert((doDataFI || doControlFI || doFrameFI) && "FI instruction types is invalid!");
        }

        bool injectDstRegs = false, injectSrcRegs = false;
        if(!FIInstTypes.empty()) {
          if(std::find(FIRegTypes.begin(), FIRegTypes.end(), "dst") != FIRegTypes.end())
            injectDstRegs = true;

          if(std::find(FIRegTypes.begin(), FIRegTypes.end(), "src") != FIRegTypes.end())
            injectSrcRegs = true;

          assert((injectDstRegs || injectSrcRegs) && "FI register types is invalid!");
        }

        if(FFEnable) {
          //dbgs() << "Fast-forwarding enabled\n";
          const TargetFaultInjection *TFI = MF.getSubtarget().getTargetFaultInjection();
          const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
          //const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
          {
            /*dbgs() << "============== MF code =============\n";
            for(auto &MBB: MF) { //DBG_SAFIRE
              MBB.dump();
            }
            dbgs() << "========== End of MF code ==========\n";*/
            // There should be an 1-to-1 correspondence between MBBs and CloneMBBs
            // Keep a list of original MBBs (first) with their Clone (second)
            SmallVector< std::pair<MachineBasicBlock *, MachineBasicBlock *>, 32> MBBs;
            for(auto &MBB: MF) {
              MachineBasicBlock *CloneMBB = MF.CreateMachineBasicBlock(nullptr);
              CloneMBB->setIsEHPad( MBB.isEHPad() );
              MBBs.push_back(std::make_pair(&MBB, CloneMBB));
            }

            // Populate detach clones
            for(auto MBBPair : MBBs) {
              // Create a new, no-FI MBB to fast-forward execution when no more faults inject.
              // Saves the overhead within a function, but selMBB will be invoked again when 
              // calling a function, at the function head only. No way (yet) of FF function calls
              MachineBasicBlock *MBB = MBBPair.first;
              MachineBasicBlock *CloneMBB = MBBPair.second;

              MF.push_back(CloneMBB);

              // Copy instructions
              for(MachineBasicBlock::instr_iterator Iter = MBB->instr_begin(); Iter != MBB->instr_end(); Iter++) {
                MachineInstr *MI = &*Iter;
                // XXX: avoid copying non-duplicatable instructions: pseudo instructions, DBG or EH labels
                if(MI->isNotDuplicable())
                  continue;
                MachineInstr *CopyMI = MF.CloneMachineInstr(MI);
                CloneMBB->push_back(CopyMI);
              }
              // Copy successors
              for(MachineBasicBlock::succ_iterator Iter = MBB->succ_begin(); Iter != MBB->succ_end(); Iter++)
                CloneMBB->addSuccessor(*Iter);

              /*dbgs() << "Original:";
                MBB->dump();
                dbgs() << "Clone:";
                CloneMBB->dump();*/ //DBG_SAFIRE
            }

            // Move the successors to other CloneMBBs to skip selMBB calls
            for(auto MBBPair: MBBs) {
              MachineBasicBlock *CloneMBB = MBBPair.second;
              for(MachineBasicBlock::succ_iterator Iter = CloneMBB->succ_begin(); Iter != CloneMBB->succ_end(); Iter++) {
                // Find MBB to redirect to its clone
                auto it = std::find_if( MBBs.begin(), MBBs.end(),
                    [Iter](const std::pair<MachineBasicBlock *, MachineBasicBlock *> &element){ return element.first == *Iter;} );
                assert(it != MBBs.end() && "Could not find successor in MBBs!\n");
                CloneMBB->ReplaceUsesOfBlockWith(*Iter, it->second);
              }
              //CloneMBB->dump(); //DBG_SAFIRE
            }

            MachineBasicBlock *TBB = nullptr, *FBB = nullptr;
            SmallVector<MachineOperand, 4> Cond;
            for(auto MBBPair: MBBs) {
              //dbgs() << "Terminator update for:";
              MachineBasicBlock *CloneMBB = MBBPair.second;
              /*dbgs() << "==== UPDATE TERMINATORS ====\n";
              dbgs() << "==== original ====\n";
              MBBPair.first->dump();
              dbgs() << "==== end original ====\n";
              dbgs() << "==== clone ====\n";
              CloneMBB->dump(); //DBG_SAFIRE
              dbgs() << "==== end clone ====\n";
              dbgs() << "==== END UPDATE TERMINATORS ====\n";*/ //DBG_SAFIRE
              if( !TII.analyzeBranch(*CloneMBB, TBB, FBB, Cond) )
                CloneMBB->updateTerminator();
            }

            // Populate the vector of FI Target MachineBasicBlocks
            SmallVector< std::pair<MachineBasicBlock *, MachineBasicBlock *>, 32> TargetMBBs;
            dbgs() << "VERSION 14\n"; //DBG_SAFIRE
            for(auto MBBPair: MBBs) {
              MachineBasicBlock *MBB = MBBPair.first;
              //dbgs() << "Target MBB: " << MBB.getSymbol()->getName() << "\n";

              // XXX: Splitting hurts performance by creating artificial basic blocks. With splitting, instruction
              // indexing follows execution order. Without splitting, instruction indexing follows BB order. Random
              // injection is unaffected. Vote for performance for now.
#if 0
              // XXX: Split MBB in Call instruction to count instructions correctly across function calls.
              // The original basic block is split in two blocks: before and including the call instruction 
              // and after the call instruction. The new block after the call is going to be examined and 
              // added to the potential iterations in the next iteration of the outer loop.
              for(auto &MI : MBB) {
                /*if(MI.isCall()) { //DBG_SAFIRE
                  dbgs() << "MI.isTerminator: " << MI.isTerminator() << "\n"; //DBG_SAFIRE
                  dbgs() << "MI.isLast: " << ( std::next(MI.getIterator()) == MBB.instr_end() ) << "\n"; //DBG_SAFIRE
                  }*/
                if( MI.isCall() && !( MI.isTerminator() || ( std::next(MI.getIterator()) == MBB.instr_end() ) ) ) {
                  /*dbgs() << "ORIGINAL\n"; //DBG_SAFIRE
                    MBB.dump();
                    dbgs() << "====================================\n";*/

                  MachineBasicBlock *SplitMBB = MF.CreateMachineBasicBlock(nullptr);
                  MF.insert(++MBB.getIterator(), SplitMBB);

                  SplitMBB->splice(SplitMBB->end(), &MBB, std::next(MI.getIterator()), MBB.end());
                  SplitMBB->transferSuccessors(&MBB);
                  MBB.addSuccessor(SplitMBB);
                  //TII.InsertBranch(MBB, SplitMBB, nullptr, None, DebugLoc()); // XXX: is this needed?
                  /*dbgs() << "====================================\n"; //DBG_SAFIRE
                    dbgs() << "NEW\n";
                    MBB.dump();
                    SplitMBB->dump();
                    dbgs() << "====================================\n"; //DBG_SAFIRE*/

                  MBB.updateTerminator();
                  SplitMBB->updateTerminator();
                  /*dbgs() << "====================================\n"; //DBG_SAFIRE
                    dbgs() << "UPDATED\n";
                    MBB.dump();
                    SplitMBB->dump();
                    dbgs() << "====================================\n"; //DBG_SAFIRE*/

                  break;
                }
              }
#endif

              // vecFIInstr stores FI instructions and vector of eligible operands
              SmallVector< std::pair< MachineInstr *, SmallVector< MachineOperand *, 4 > >, 32> vecFIInstr;
              // XXX: If no target instructions, skip from instrumentation
              uint64_t InstrCount;
              uint64_t TargetInstrCount;
              std::tie( InstrCount, TargetInstrCount ) = findTargetInstructionsPair(vecFIInstr, *MBB, doDataFI, doControlFI, doFrameFI, injectDstRegs, injectSrcRegs);
              // Skip non-fi targeted blocks
              if( TargetInstrCount > 0)
                TargetMBBs.push_back(MBBPair);
              else {
                //dbgs() << "SKIPPING MBB: " << MBB.getSymbol()->getName() << "\n"; //DBG_SAFIRE
                //MBB.dump();
              }
            }

            for(auto MBBPair: TargetMBBs) {
              MachineBasicBlock *MBB = MBBPair.first;
              MachineBasicBlock *CloneMBB = MBBPair.second;
              // Create MBBs for jump detach, jump fi, original BB instrumented, copy Inst instrumented
              MachineBasicBlock *JmpDetachMBB = MF.CreateMachineBasicBlock(nullptr);
              MachineBasicBlock *JmpFIMBB = MF.CreateMachineBasicBlock(nullptr);
              MachineBasicBlock *CopyMBB = MF.CreateMachineBasicBlock(nullptr);
              MachineBasicBlock *OriginalMBB = MF.CreateMachineBasicBlock(nullptr);

              // Add SelMBB before MBB
              MachineFunction::iterator MBBI = MBB->getIterator();
              MF.insert(++MBBI, JmpDetachMBB);
              MBBI = JmpDetachMBB->getIterator();
              MF.insert(++MBBI, JmpFIMBB);
              MBBI = JmpFIMBB->getIterator();
              MF.insert(++MBBI, OriginalMBB);
              MBBI = OriginalMBB->getIterator();
              // Add CopyMBB after OriginalMBB
              //MF.insert(++MBBI, CopyMBB);
              // If CopyMBB at function's end we save a jump from OriginalMBB (common case), CopyMBB jumps back
              MF.insert(MF.end(), CopyMBB);

              OriginalMBB->splice(OriginalMBB->end(), MBB, MBB->begin(), MBB->end());
              OriginalMBB->transferSuccessors(MBB);

              MBB->addSuccessor(JmpDetachMBB);
              MBB->addSuccessor(JmpFIMBB);

              JmpDetachMBB->addSuccessor(CloneMBB);

              JmpFIMBB->addSuccessor(OriginalMBB);
              JmpFIMBB->addSuccessor(CopyMBB);

              // Copy instructions
              for(MachineBasicBlock::instr_iterator Iter = OriginalMBB->instr_begin(); Iter != OriginalMBB->instr_end(); Iter++) {
                MachineInstr *MI = &*Iter;
                // XXX: avoid copying non-duplicatable instructions: pseudo instructions, DBG or EH labels
                if(MI->isNotDuplicable())
                  continue;
                MachineInstr *CopyMI = MF.CloneMachineInstr(MI);
                CopyMBB->push_back(CopyMI);
              }

              // Copy successors
              for(MachineBasicBlock::succ_iterator Iter = OriginalMBB->succ_begin(); Iter != OriginalMBB->succ_end(); Iter++)
                CopyMBB->addSuccessor(*Iter);

              //SmallVector<MachineInstr *, 32> vecFIInstr;
              SmallVector< std::pair< MachineInstr *, SmallVector< MachineOperand *, 4 > >, 32> vecFIInstr;
              uint64_t InstrCount;
              uint64_t TargetInstrCount;
              // XXX: Run again on the CopyMBB this same. Result is the same but on the copied instruction stream
              // TODO: analyze CopyMBB before the updateTerminators()!
              std::tie( InstrCount, TargetInstrCount ) = findTargetInstructionsPair(vecFIInstr, *CopyMBB, doDataFI, doControlFI, doFrameFI, injectDstRegs, injectSrcRegs);
              // XXX: Note TotalInstrCount might not be the same in FF because not all blocks are considered
              FuncInstrCount += InstrCount;
              FuncTargetInstrCount += TargetInstrCount;

              assert(TargetInstrCount > 0 && "TargetInstrCount cannot be 0!\n");

              if(SaveInstrEnable) {
                for(auto I : vecFIInstr) {
                  MachineInstr *MI = I.first;
                  std::string str;
                  llvm::raw_string_ostream rso(str);
                  MI->print(rso);
                  InstrumentFile << rso.str();
                }
              }
              // XXX: Note FF method may count less total instructions because it doesn't process MBBs with no Targets
              /*dbgs() << "=============================================\n";
              dbgs() << "MBB: " << MBB->getSymbol()->getName() << " InstrCount: " << InstrCount << ", TargetInstrCount:" << TargetInstrCount << "\n";
              dbgs() << "=============================================\n";*/ //DBG_SAFIRE

              // XXX: instrument instrutions before injectMBB
              instrumentInstructionsInMachineBasicBlock(vecFIInstr, MF);

              // XXX: injectMachineBlock after OriginalMBB and CopyMBB have their instructions populated
              // because it needs to add a preamble for restoring the context state after selMBB
              TFI->injectMachineBasicBlock(*MBB, *JmpDetachMBB, *JmpFIMBB, *OriginalMBB, *CopyMBB, TargetInstrCount);

              // Insert the branch since JmpDetachMBB jumps unconditionally, hence no target specific codegen
              // XXX: MUST happen after injectMachineBasicBlock to be added as the terminator
              TII.InsertBranch(*JmpDetachMBB, CloneMBB, nullptr, None, DebugLoc());

              MBB->updateTerminator();

              // XXX: Need to update terminators *ONLY* if it's an analyzable branch (e.g., NOT an indirect branch)
              MachineBasicBlock *TBB = nullptr, *FBB = nullptr;
              SmallVector<MachineOperand, 4> Cond;
              if( !TII.analyzeBranch(*OriginalMBB, TBB, FBB, Cond) )
                OriginalMBB->updateTerminator();

              TBB = nullptr; FBB = nullptr; Cond.clear();
              if( !TII.analyzeBranch(*CopyMBB, TBB, FBB, Cond) )
                CopyMBB->updateTerminator();

              TBB = nullptr; FBB = nullptr; Cond.clear();
              if( !TII.analyzeBranch(*CopyMBB, TBB, FBB, Cond) )
                JmpDetachMBB->updateTerminator();

              TBB = nullptr; FBB = nullptr; Cond.clear();
              if( !TII.analyzeBranch(*CopyMBB, TBB, FBB, Cond) )
                JmpFIMBB->updateTerminator();

              /*CopyMBB->dump();
                dbgs() << "ORIGINAL\n";
                MBB.dump(); */ //DBG_SAFIRE
            }
          }

          dbgs() << "=============================================\n";
          dbgs() << "MF: " << MF.getName() << " FuncInstrCount: " << FuncInstrCount << ", FuncTargetInstrCount:" << FuncTargetInstrCount << "\n";
          dbgs() << "=============================================\n";
        }
        else {
          SmallVector<MachineBasicBlock *, 8> TargetMBBs;
            for(auto &MBB: MF) {
              TargetMBBs.push_back(&MBB);
            }
            for(auto MBB: TargetMBBs) {
              //SmallVector<MachineInstr *, 32> vecFIInstr;
              SmallVector< std::pair< MachineInstr *, SmallVector< MachineOperand *, 4 > >, 32> vecFIInstr;
              uint64_t InstrCount;
              uint64_t TargetInstrCount;
              std::tie( InstrCount, TargetInstrCount ) = findTargetInstructionsPair(vecFIInstr, *MBB, doDataFI, doControlFI, doFrameFI, injectDstRegs, injectSrcRegs);
              //dbgs() << "TargetInstrCount: " << TargetInstrCount << "\n"; //DBG_SAFIRE
              //MBB->dump(); // DBG_SAFIRE
              FuncInstrCount += InstrCount;
              FuncTargetInstrCount += TargetInstrCount;

              /*dbgs() << "=============================================\n";
              dbgs() << "MBB: " << MBB->getSymbol()->getName() << " InstrCount: " << InstrCount << ", TargetInstrCount:" << TargetInstrCount << "\n";
              dbgs() << "=============================================\n";*/

              instrumentInstructionsInMachineBasicBlock(vecFIInstr, MF);
            }

            dbgs() << "=============================================\n";
            dbgs() << "MF: " << MF.getName() << " FuncInstrCount: " << FuncInstrCount << ", FuncTargetInstrCount:" << FuncTargetInstrCount << "\n";
            dbgs() << "=============================================\n";
        }

        TotalInstrCount += FuncInstrCount;
        TotalTargetInstrCount += FuncTargetInstrCount;
      }

      return true;
    }
  };

} // end anonymous namespace

char MCFaultInjectionPass::ID = 0;
char &llvm::MCFaultInjectionPassID = MCFaultInjectionPass::ID;

INITIALIZE_PASS(MCFaultInjectionPass, "mc-fi", "MC FI Pass", false, false)

namespace llvm {

  MachineFunctionPass *createMCFaultInjectionPass() {
    return new MCFaultInjectionPass();
  }

}


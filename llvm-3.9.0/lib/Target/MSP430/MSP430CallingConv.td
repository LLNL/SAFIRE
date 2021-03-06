//==- MSP430CallingConv.td - Calling Conventions for MSP430 -*- tablegen -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This describes the calling conventions for MSP430 architecture.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// MSP430 Return Value Calling Convention
//===----------------------------------------------------------------------===//
def RetCC_MSP430 : CallingConv<[
  // i8 are returned in registers R15B, R14B, R13B, R12B
  CCIfType<[i8], CCAssignToReg<[R15B, R14B, R13B, R12B]>>,

  // i16 are returned in registers R15, R14, R13, R12
  CCIfType<[i16], CCAssignToReg<[R15, R14, R13, R12]>>
]>;

//===----------------------------------------------------------------------===//
// MSP430 Argument Calling Conventions
//===----------------------------------------------------------------------===//
def CC_MSP430_AssignStack : CallingConv<[
  // Pass by value if the byval attribute is given
  CCIfByVal<CCPassByVal<2, 2>>,

  // Promote i8 arguments to i16.
  CCIfType<[i8], CCPromoteToType<i16>>,

  // Integer values get stored in stack slots that are 2 bytes in
  // size and 2-byte aligned.
  CCIfType<[i16], CCAssignToStack<2, 2>>
]>;


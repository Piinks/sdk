// Copyright (c) 2018, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef RUNTIME_VM_CONSTANTS_KBC_H_
#define RUNTIME_VM_CONSTANTS_KBC_H_

#include "platform/assert.h"
#include "platform/globals.h"
#include "platform/utils.h"

namespace dart {

// clang-format off
// List of KernelBytecode instructions.
//
// INTERPRETER STATE
//
//      current frame info (see stack_frame_kbc.h for layout)
//        v-----^-----v
//   ~----+----~ ~----+-------+-------+-~ ~-+-------+-------+-~
//   ~    |    ~ ~    | FP[0] | FP[1] | ~ ~ | SP[-1]| SP[0] |
//   ~----+----~ ~----+-------+-------+-~ ~-+-------+-------+-~
//                    ^                             ^
//                    FP                            SP
//
//
// The state of execution is captured in few interpreter registers:
//
//   FP - base of the current frame
//   SP - top of the stack (TOS) for the current frame
//   PP - object pool for the currently execution function
//
// Frame info stored below FP additionally contains pointers to the currently
// executing function and code (see stack_frame_dbc.h for more information).
//
// In the unoptimized code most of bytecodes take operands implicitly from
// stack and store results again on the stack. Constant operands are usually
// taken from the object pool by index.
//
// ENCODING
//
// Each instruction starts with opcode byte. Certain instructions have
// wide encoding variant. In such case, the least significant bit of opcode is
// not set for compact variant and set for wide variant.
//
// The following operand encodings are used:
//
//   0........8.......16.......24.......32.......40.......48
//   +--------+
//   | opcode |                              0: no operands
//   +--------+
//
//   +--------+--------+
//   | opcode |    A   |                     A: unsigned 8-bit operand
//   +--------+--------+
//
//   +--------+--------+
//   | opcode |   D    |                     D: unsigned 8/32-bit operand
//   +--------+--------+
//
//   +--------+----------------------------------+
//   | opcode |                D                 |            D (wide)
//   +--------+----------------------------------+
//
//   +--------+--------+
//   | opcode |   X    |                     X: signed 8/32-bit operand
//   +--------+--------+
//
//   +--------+----------------------------------+
//   | opcode |                X                 |            X (wide)
//   +--------+----------------------------------+
//
//   +--------+--------+
//   | opcode |    T   |                     T: signed 8/24-bit operand
//   +--------+--------+
//
//   +--------+--------------------------+
//   | opcode |            T             |   T (wide)
//   +--------+--------------------------+
//
//   +--------+--------+--------+
//   | opcode |    A   |   E    |            A_E: unsigned 8-bit operand and
//   +--------+--------+--------+                 unsigned 8/32-bit operand
//
//   +--------+--------+----------------------------------+
//   | opcode |    A   |                 E                |   A_E (wide)
//   +--------+--------+----------------------------------+
//
//   +--------+--------+--------+
//   | opcode |    A   |   Y    |            A_Y: unsigned 8-bit operand and
//   +--------+--------+--------+                 signed 8/32-bit operand
//
//   +--------+--------+----------------------------------+
//   | opcode |    A   |                 Y                |   A_Y (wide)
//   +--------+--------+----------------------------------+
//
//   +--------+--------+--------+
//   | opcode |    D   |   F    |            D_F: unsigned 8/32-bit operand and
//   +--------+--------+--------+                 unsigned 8-bit operand
//
//   +--------+----------------------------------+--------+
//   | opcode |                 D                |    F   |   D_F (wide)
//   +--------+----------------------------------+--------+
//
//   +--------+--------+--------+--------+
//   | opcode |    A   |    B   |    C   |   A_B_C: 3 unsigned 8-bit operands
//   +--------+--------+--------+--------+
//
//
// INSTRUCTIONS
//
//  - Trap
//
//    Unreachable instruction.
//
//  - Entry rD
//
//    Function prologue for the function
//        rD - number of local slots to reserve;
//
//  - EntryFixed A, D
//
//    Function prologue for functions without optional arguments.
//    Checks number of arguments.
//        A - expected number of positional arguments;
//        D - number of local slots to reserve;
//
//  - EntryOptional A, B, C
//
//    Function prologue for the function with optional or named arguments:
//        A - expected number of positional arguments;
//        B - number of optional arguments;
//        C - number of named arguments;
//
//    Only one of B and C can be not 0.
//
//    If B is not 0 then EntryOptional bytecode is followed by B LoadConstant
//    bytecodes specifying default values for optional arguments.
//
//    If C is not 0 then EntryOptional is followed by 2 * C LoadConstant
//    bytecodes.
//    Bytecode at 2 * i specifies name of the i-th named argument and at
//    2 * i + 1 default value. rA part of the LoadConstant bytecode specifies
//    the location of the parameter on the stack. Here named arguments are
//    sorted alphabetically to enable linear matching similar to how function
//    prologues are implemented on other architectures.
//
//    Note: Unlike Entry bytecode EntryOptional does not setup the frame for
//    local variables this is done by a separate bytecode Frame, which should
//    follow EntryOptional and its LoadConstant instructions.
//
//  - LoadConstant rA, D
//
//    Used in conjunction with EntryOptional instruction to describe names and
//    default values of optional parameters.
//
//  - Frame D
//
//    Reserve and initialize with null space for D local variables.
//
//  - CheckFunctionTypeArgs A, D
//
//    Check for a passed-in type argument vector of length A and
//    store it at FP[D].
//
//  - CheckStack A
//
//    Compare SP against isolate stack limit and call StackOverflow handler if
//    necessary. Should be used in prologue (A = 0), or at the beginning of
//    a loop with depth A.
//
//  - Allocate D
//
//    Allocate object of class PP[D] with no type arguments.
//
//  - AllocateT
//
//    Allocate object of class SP[0] with type arguments SP[-1].
//
//  - CreateArrayTOS
//
//    Allocate array of length SP[0] with type arguments SP[-1].
//
//  - AllocateContext A, D
//
//    Allocate Context object holding D context variables.
//    A is a static ID of the context. Static ID of a context may be used to
//    disambiguate accesses to different context objects.
//    Context objects with the same ID should have the same number of
//    context variables.
//
//  - CloneContext A, D
//
//    Clone Context object SP[0] holding D context variables.
//    A is a static ID of the context. Cloned context has the same ID.
//
//  - LoadContextParent
//
//    Load parent from context SP[0].
//
//  - StoreContextParent
//
//    Store context SP[0] into `parent` field of context SP[-1].
//
//  - LoadContextVar A, D
//
//    Load value from context SP[0] at index D.
//    A is a static ID of the context.
//
//  - StoreContextVar A, D
//
//    Store value SP[0] into context SP[-1] at index D.
//    A is a static ID of the context.
//
//  - PushConstant D
//
//    Push value at index D from constant pool onto the stack.
//
//  - PushNull
//
//    Push `null` onto the stack.
//
//  - PushTrue
//
//    Push `true` onto the stack.
//
//  - PushFalse
//
//    Push `false` onto the stack.
//
//  - PushInt rX
//
//    Push int rX onto the stack.
//
//  - Drop1
//
//    Drop 1 value from the stack
//
//  - Push rX
//
//    Push FP[rX] to the stack.
//
//  - StoreLocal rX; PopLocal rX
//
//    Store top of the stack into FP[rX] and pop it if needed.
//
//  - LoadFieldTOS D
//
//    Push value at offset (in words) PP[D] from object SP[0].
//
//  - StoreFieldTOS D
//
//    Store value SP[0] into object SP[-1] at offset (in words) PP[D].
//
//  - StoreIndexedTOS
//
//    Store SP[0] into array SP[-2] at index SP[-1]. No typechecking is done.
//    SP[-2] is assumed to be a RawArray, SP[-1] to be a smi.
//
//  - PushStatic D
//
//    Pushes value of the static field PP[D] on to the stack.
//
//  - StoreStaticTOS D
//
//    Stores TOS into the static field PP[D].
//
//  - Jump target
//
//    Jump to the given target. Target is specified as offset from the PC of the
//    jump instruction.
//
//  - JumpIfNoAsserts target
//
//    Jump to the given target if assertions are not enabled.
//    Target is specified as offset from the PC of the jump instruction.
//
//  - JumpIfNotZeroTypeArgs target
//
//    Jump to the given target if number of passed function type
//    arguments is not zero.
//    Target is specified as offset from the PC of the jump instruction.
//
//  - JumpIfEqStrict target; JumpIfNeStrict target
//
//    Jump to the given target if SP[-1] is the same (JumpIfEqStrict) /
//    not the same (JumpIfNeStrict) object as SP[0].
//
//  - JumpIfTrue target; JumpIfFalse target
//  - JumpIfNull target; JumpIfNotNull target
//
//    Jump to the given target if SP[0] is true/false/null/not null.
//
//  - IndirectStaticCall ArgC, D
//
//    Invoke the function given by the ICData in SP[0] with arguments
//    SP[-(1+ArgC)], ..., SP[-1] and argument descriptor PP[D], which
//    indicates whether the first argument is a type argument vector.
//
//  - DirectCall ArgC, D
//
//    Invoke the function PP[D] with arguments
//    SP[-(ArgC-1)], ..., SP[0] and argument descriptor PP[D+1].
//
//  - InterfaceCall ArgC, D
//
//    Lookup and invoke method using ICData in PP[D]
//    with arguments SP[-(1+ArgC)], ..., SP[-1].
//    Method has to be declared (explicitly or implicitly) in an interface
//    implemented by a receiver, and passed arguments are valid for the
//    interface method declaration.
//    The ICData indicates whether the first argument is a type argument vector.
//
//  - UncheckedInterfaceCall ArgC, D
//
//    Same as InterfaceCall, but can omit type checks of generic-covariant
//    parameters.
//
//  - DynamicCall ArgC, D
//
//    Lookup and invoke method using ICData in PP[D]
//    with arguments SP[-(1+ArgC)], ..., SP[-1].
//    The ICData indicates whether the first argument is a type argument vector.
//
//  - NativeCall D
//
//    Invoke native function described by array at pool[D].
//    array[0] is wrapper, array[1] is function, array[2] is argc_tag.
//
//  - ReturnTOS
//
//    Return to the caller using a value from the top-of-stack as a result.
//
//    Note: return instruction knows how many arguments to remove from the
//    stack because it can look at the call instruction at caller's PC and
//    take argument count from it.
//
//  - AssertAssignable A, D
//
//    Assert that instance SP[-4] is assignable to variable named SP[0] of
//    type SP[-1] with instantiator type arguments SP[-3] and function type
//    arguments SP[-2] using SubtypeTestCache PP[D].
//    If A is 1, then the instance may be a Smi.
//
//    Instance remains on stack. Other arguments are consumed.
//
//  - AssertBoolean A
//
//    Assert that TOS is a boolean (A = 1) or that TOS is not null (A = 0).
//
//  - AssertSubtype
//
//    Assert that one type is a subtype of another.  Throws a TypeError
//    otherwise.  The stack has the following arguments on it:
//
//        SP[-4]  instantiator type args
//        SP[-3]  function type args
//        SP[-2]  sub_type
//        SP[-1]  super_type
//        SP[-0]  dst_name
//
//    All 5 arguments are consumed from the stack and no results is pushed.
//
//  - LoadTypeArgumentsField D
//
//    Load instantiator type arguments from an instance SP[0].
//    PP[D] = offset (in words) of type arguments field corresponding
//    to an instance's class.
//
//  - InstantiateType D
//
//    Instantiate type PP[D] with instantiator type arguments SP[-1] and
//    function type arguments SP[0].
//
//  - InstantiateTypeArgumentsTOS A, D
//
//    Instantiate type arguments PP[D] with instantiator type arguments SP[-1]
//    and function type arguments SP[0]. A != 0 indicates that resulting type
//    arguments are all dynamic if both instantiator and function type
//    arguments are all dynamic.
//
//  - Throw A
//
//    Throw (Rethrow if A != 0) exception. Exception object and stack object
//    are taken from TOS.
//
//  - MoveSpecial A, rX
//
//    Copy value from special variable to FP[rX]. Currently only
//    used to pass exception object (A = 0) and stack trace object (A = 1) to
//    catch handler.
//
//  - SetFrame A
//
//    Reinitialize SP assuming that current frame has size A.
//    Used to drop temporaries from the stack in the exception handler.
//
//  - BooleanNegateTOS
//
//    SP[0] = !SP[0]
//
//  - EqualsNull
//
//    SP[0] = (SP[0] == null) ? true : false
//
//  - NegateInt
//
//    Equivalent to invocation of unary int operator-.
//    Receiver should have static type int.
//    Check SP[0] for null; SP[0] = -SP[0].
//
//  - AddInt; SubInt; MulInt; TruncDivInt; ModInt; BitAndInt; BitOrInt;
//    BitXorInt; ShlInt; ShrInt
//
//    Equivalent to invocation of binary int operator +, -, *, ~/, %, &, |,
//    ^, << or >>. Receiver and argument should have static type int.
//    Check SP[-1] and SP[0] for null; push SP[-1] <op> SP[0].
//
//  - CompareIntEq; CompareIntGt; CompareIntLt; CompareIntGe; CompareIntLe
//
//    Equivalent to invocation of binary int operator ==, >, <, >= or <=.
//    Receiver and argument should have static type int.
//    Check SP[-1] and SP[0] for null; push SP[-1] <op> SP[0] ? true : false.
//
//  - NegateDouble
//
//    Equivalent to invocation of unary double operator-.
//    Receiver should have static type double.
//    Check SP[0] for null; SP[0] = -SP[0].
//
//  - AddDouble; SubDouble; MulDouble; DivDouble
//
//    Equivalent to invocation of binary int operator +, -, *, /.
//    Receiver and argument should have static type double.
//    Check SP[-1] and SP[0] for null; push SP[-1] <op> SP[0].
//
//  - CompareDoubleEq; CompareDoubleGt; CompareDoubleLt; CompareDoubleGe;
//    CompareDoubleLe
//
//    Equivalent to invocation of binary double operator ==, >, <, >= or <=.
//    Receiver and argument should have static type double.
//    Check SP[-1] and SP[0] for null; push SP[-1] <op> SP[0] ? true : false.
//
//  - AllocateClosure D
//
//    Allocate closure object for closure function ConstantPool[D].
//
// BYTECODE LIST FORMAT
//
// KernelBytecode list below is specified using the following format:
//
//     V(BytecodeName, OperandForm, BytecodeKind, Op1, Op2, Op3)
//
// - OperandForm specifies operand encoding and should be one of 0, A, D, X, T,
//   A_D (old), A_X (old), A_E, A_Y, D_F or A_B_C (see ENCODING section above).
//
// - BytecodeKind is one of OLD, WIDE, RESV (reserved), ORDN (ordinary)
//
// - Op1, Op2, Op3 specify operand meaning. Possible values:
//
//     ___ ignored / non-existent operand
//     num immediate operand
//     lit constant literal from object pool
//     reg register (unsigned FP relative local)
//     xeg x-register (signed FP relative local)
//     tgt jump target relative to the PC of the current instruction
//
// TODO(vegorov) jump targets should be encoded relative to PC of the next
//               instruction because PC is incremented immediately after fetch
//               and before decoding.
//
#define PUBLIC_KERNEL_BYTECODES_LIST(V)                                        \
  V(Trap_Old,                              0,  OLD, ___, ___, ___)             \
  V(Entry_Old,                             D,  OLD, num, ___, ___)             \
  V(EntryFixed_Old,                      A_D,  OLD, num, num, ___)             \
  V(EntryOptional_Old,                 A_B_C,  OLD, num, num, num)             \
  V(LoadConstant_Old,                    A_D,  OLD, reg, lit, ___)             \
  V(Frame_Old,                             D,  OLD, num, ___, ___)             \
  V(CheckFunctionTypeArgs_Old,           A_D,  OLD, num, reg, ___)             \
  V(CheckStack_Old,                        A,  OLD, num, ___, ___)             \
  V(Allocate_Old,                          D,  OLD, lit, ___, ___)             \
  V(AllocateT_Old,                         0,  OLD, ___, ___, ___)             \
  V(CreateArrayTOS_Old,                    0,  OLD, ___, ___, ___)             \
  V(AllocateContext_Old,                   D,  OLD, num, ___, ___)             \
  V(CloneContext_Old,                      D,  OLD, num, ___, ___)             \
  V(LoadContextParent_Old,                 0,  OLD, ___, ___, ___)             \
  V(StoreContextParent_Old,                0,  OLD, ___, ___, ___)             \
  V(LoadContextVar_Old,                    D,  OLD, num, ___, ___)             \
  V(StoreContextVar_Old,                   D,  OLD, num, ___, ___)             \
  V(PushConstant_Old,                      D,  OLD, lit, ___, ___)             \
  V(PushNull_Old,                          0,  OLD, ___, ___, ___)             \
  V(PushTrue_Old,                          0,  OLD, ___, ___, ___)             \
  V(PushFalse_Old,                         0,  OLD, ___, ___, ___)             \
  V(PushInt_Old,                           X,  OLD, num, ___, ___)             \
  V(Drop1_Old,                             0,  OLD, ___, ___, ___)             \
  V(Push_Old,                              X,  OLD, xeg, ___, ___)             \
  V(PopLocal_Old,                          X,  OLD, xeg, ___, ___)             \
  V(StoreLocal_Old,                        X,  OLD, xeg, ___, ___)             \
  V(LoadFieldTOS_Old,                      D,  OLD, lit, ___, ___)             \
  V(StoreFieldTOS_Old,                     D,  OLD, lit, ___, ___)             \
  V(StoreIndexedTOS_Old,                   0,  OLD, ___, ___, ___)             \
  V(PushStatic_Old,                        D,  OLD, lit, ___, ___)             \
  V(StoreStaticTOS_Old,                    D,  OLD, lit, ___, ___)             \
  V(Jump_Old,                              T,  OLD, tgt, ___, ___)             \
  V(JumpIfNoAsserts_Old,                   T,  OLD, tgt, ___, ___)             \
  V(JumpIfNotZeroTypeArgs_Old,             T,  OLD, tgt, ___, ___)             \
  V(JumpIfEqStrict_Old,                    T,  OLD, tgt, ___, ___)             \
  V(JumpIfNeStrict_Old,                    T,  OLD, tgt, ___, ___)             \
  V(JumpIfTrue_Old,                        T,  OLD, tgt, ___, ___)             \
  V(JumpIfFalse_Old,                       T,  OLD, tgt, ___, ___)             \
  V(JumpIfNull_Old,                        T,  OLD, tgt, ___, ___)             \
  V(JumpIfNotNull_Old,                     T,  OLD, tgt, ___, ___)             \
  V(Unused00_Old,                          0, RESV, num, num, ___)             \
  V(InterfaceCall_Old,                   A_D,  OLD, num, num, ___)             \
  V(DynamicCall_Old,                     A_D,  OLD, num, num, ___)             \
  V(NativeCall_Old,                        D,  OLD, lit, ___, ___)             \
  V(ReturnTOS_Old,                         0,  OLD, ___, ___, ___)             \
  V(AssertAssignable_Old,                A_D,  OLD, num, lit, ___)             \
  V(AssertBoolean_Old,                     A,  OLD, num, ___, ___)             \
  V(AssertSubtype_Old,                     0,  OLD, ___, ___, ___)             \
  V(LoadTypeArgumentsField_Old,            D,  OLD, lit, ___, ___)             \
  V(InstantiateType_Old,                   D,  OLD, lit, ___, ___)             \
  V(InstantiateTypeArgumentsTOS_Old,     A_D,  OLD, num, lit, ___)             \
  V(Throw_Old,                             A,  OLD, num, ___, ___)             \
  V(MoveSpecial_Old,                     A_X,  OLD, num, xeg, ___)             \
  V(SetFrame_Old,                          A,  OLD, num, ___, num)             \
  V(BooleanNegateTOS_Old,                  0,  OLD, ___, ___, ___)             \
  V(EqualsNull_Old,                        0,  OLD, ___, ___, ___)             \
  V(NegateInt_Old,                         0,  OLD, ___, ___, ___)             \
  V(AddInt_Old,                            0,  OLD, ___, ___, ___)             \
  V(SubInt_Old,                            0,  OLD, ___, ___, ___)             \
  V(MulInt_Old,                            0,  OLD, ___, ___, ___)             \
  V(TruncDivInt_Old,                       0,  OLD, ___, ___, ___)             \
  V(ModInt_Old,                            0,  OLD, ___, ___, ___)             \
  V(BitAndInt_Old,                         0,  OLD, ___, ___, ___)             \
  V(BitOrInt_Old,                          0,  OLD, ___, ___, ___)             \
  V(BitXorInt_Old,                         0,  OLD, ___, ___, ___)             \
  V(ShlInt_Old,                            0,  OLD, ___, ___, ___)             \
  V(ShrInt_Old,                            0,  OLD, ___, ___, ___)             \
  V(CompareIntEq_Old,                      0,  OLD, ___, ___, ___)             \
  V(CompareIntGt_Old,                      0,  OLD, ___, ___, ___)             \
  V(CompareIntLt_Old,                      0,  OLD, ___, ___, ___)             \
  V(CompareIntGe_Old,                      0,  OLD, ___, ___, ___)             \
  V(CompareIntLe_Old,                      0,  OLD, ___, ___, ___)             \
  V(DirectCall_Old,                      A_D,  OLD, num, num, ___)             \
  V(AllocateClosure_Old,                   D,  OLD, lit, ___, ___)             \
  V(UncheckedInterfaceCall_Old,          A_D,  OLD, num, num, ___)             \
  V(NegateDouble_Old,                      0,  OLD, ___, ___, ___)             \
  V(AddDouble_Old,                         0,  OLD, ___, ___, ___)             \
  V(SubDouble_Old,                         0,  OLD, ___, ___, ___)             \
  V(MulDouble_Old,                         0,  OLD, ___, ___, ___)             \
  V(DivDouble_Old,                         0,  OLD, ___, ___, ___)             \
  V(CompareDoubleEq_Old,                   0,  OLD, ___, ___, ___)             \
  V(CompareDoubleGt_Old,                   0,  OLD, ___, ___, ___)             \
  V(CompareDoubleLt_Old,                   0,  OLD, ___, ___, ___)             \
  V(CompareDoubleGe_Old,                   0,  OLD, ___, ___, ___)             \
  V(CompareDoubleLe_Old,                   0,  OLD, ___, ___, ___)             \
  V(Trap,                                  0, ORDN, ___, ___, ___)             \
  V(Entry,                                 D, ORDN, num, ___, ___)             \
  V(Entry_Wide,                            D, WIDE, num, ___, ___)             \
  V(EntryFixed,                          A_E, ORDN, num, num, ___)             \
  V(EntryFixed_Wide,                     A_E, WIDE, num, num, ___)             \
  V(EntryOptional,                     A_B_C, ORDN, num, num, num)             \
  V(Unused00,                              0, RESV, ___, ___, ___)             \
  V(LoadConstant,                        A_E, ORDN, reg, lit, ___)             \
  V(LoadConstant_Wide,                   A_E, WIDE, reg, lit, ___)             \
  V(Frame,                                 D, ORDN, num, ___, ___)             \
  V(Frame_Wide,                            D, WIDE, num, ___, ___)             \
  V(CheckFunctionTypeArgs,               A_E, ORDN, num, reg, ___)             \
  V(CheckFunctionTypeArgs_Wide,          A_E, WIDE, num, reg, ___)             \
  V(CheckStack,                            A, ORDN, num, ___, ___)             \
  V(Unused01,                              0, RESV, ___, ___, ___)             \
  V(Unused02,                              0, RESV, ___, ___, ___)             \
  V(Unused03,                              0, RESV, ___, ___, ___)             \
  V(Allocate,                              D, ORDN, lit, ___, ___)             \
  V(Allocate_Wide,                         D, WIDE, lit, ___, ___)             \
  V(AllocateT,                             0, ORDN, ___, ___, ___)             \
  V(CreateArrayTOS,                        0, ORDN, ___, ___, ___)             \
  V(AllocateClosure,                       D, ORDN, lit, ___, ___)             \
  V(AllocateClosure_Wide,                  D, WIDE, lit, ___, ___)             \
  V(AllocateContext,                     A_E, ORDN, num, ___, ___)             \
  V(AllocateContext_Wide,                A_E, WIDE, num, ___, ___)             \
  V(CloneContext,                        A_E, ORDN, num, ___, ___)             \
  V(CloneContext_Wide,                   A_E, WIDE, num, ___, ___)             \
  V(LoadContextParent,                     0, ORDN, ___, ___, ___)             \
  V(StoreContextParent,                    0, ORDN, ___, ___, ___)             \
  V(LoadContextVar,                      A_E, ORDN, num, ___, ___)             \
  V(LoadContextVar_Wide,                 A_E, WIDE, num, ___, ___)             \
  V(Unused04,                              0, RESV, ___, ___, ___)             \
  V(Unused05,                              0, RESV, ___, ___, ___)             \
  V(StoreContextVar,                     A_E, ORDN, num, ___, ___)             \
  V(StoreContextVar_Wide,                A_E, WIDE, num, ___, ___)             \
  V(PushConstant,                          D, ORDN, lit, ___, ___)             \
  V(PushConstant_Wide,                     D, WIDE, lit, ___, ___)             \
  V(Unused06,                              0, RESV, ___, ___, ___)             \
  V(Unused07,                              0, RESV, ___, ___, ___)             \
  V(PushTrue,                              0, ORDN, ___, ___, ___)             \
  V(PushFalse,                             0, ORDN, ___, ___, ___)             \
  V(PushInt,                               X, ORDN, num, ___, ___)             \
  V(PushInt_Wide,                          X, WIDE, num, ___, ___)             \
  V(Unused08,                              0, RESV, ___, ___, ___)             \
  V(Unused09,                              0, RESV, ___, ___, ___)             \
  V(Unused10,                              0, RESV, ___, ___, ___)             \
  V(Unused11,                              0, RESV, ___, ___, ___)             \
  V(PushNull,                              0, ORDN, ___, ___, ___)             \
  V(Drop1,                                 0, ORDN, ___, ___, ___)             \
  V(Push,                                  X, ORDN, xeg, ___, ___)             \
  V(Push_Wide,                             X, WIDE, xeg, ___, ___)             \
  V(Unused12,                              0, RESV, ___, ___, ___)             \
  V(Unused13,                              0, RESV, ___, ___, ___)             \
  V(Unused14,                              0, RESV, ___, ___, ___)             \
  V(Unused15,                              0, RESV, ___, ___, ___)             \
  V(Unused16,                              0, RESV, ___, ___, ___)             \
  V(Unused17,                              0, RESV, ___, ___, ___)             \
  V(PopLocal,                              X, ORDN, xeg, ___, ___)             \
  V(PopLocal_Wide,                         X, WIDE, xeg, ___, ___)             \
  V(Unused18,                              0, RESV, ___, ___, ___)             \
  V(Unused19,                              0, RESV, ___, ___, ___)             \
  V(StoreLocal,                            X, ORDN, xeg, ___, ___)             \
  V(StoreLocal_Wide,                       X, WIDE, xeg, ___, ___)             \
  V(LoadFieldTOS,                          D, ORDN, lit, ___, ___)             \
  V(LoadFieldTOS_Wide,                     D, WIDE, lit, ___, ___)             \
  V(StoreFieldTOS,                         D, ORDN, lit, ___, ___)             \
  V(StoreFieldTOS_Wide,                    D, WIDE, lit, ___, ___)             \
  V(StoreIndexedTOS,                       0, ORDN, ___, ___, ___)             \
  V(Unused20,                              0, RESV, ___, ___, ___)             \
  V(PushStatic,                            D, ORDN, lit, ___, ___)             \
  V(PushStatic_Wide,                       D, WIDE, lit, ___, ___)             \
  V(StoreStaticTOS,                        D, ORDN, lit, ___, ___)             \
  V(StoreStaticTOS_Wide,                   D, WIDE, lit, ___, ___)             \
  V(Jump,                                  T, ORDN, tgt, ___, ___)             \
  V(Jump_Wide,                             T, WIDE, tgt, ___, ___)             \
  V(JumpIfNoAsserts,                       T, ORDN, tgt, ___, ___)             \
  V(JumpIfNoAsserts_Wide,                  T, WIDE, tgt, ___, ___)             \
  V(JumpIfNotZeroTypeArgs,                 T, ORDN, tgt, ___, ___)             \
  V(JumpIfNotZeroTypeArgs_Wide,            T, WIDE, tgt, ___, ___)             \
  V(JumpIfEqStrict,                        T, ORDN, tgt, ___, ___)             \
  V(JumpIfEqStrict_Wide,                   T, WIDE, tgt, ___, ___)             \
  V(JumpIfNeStrict,                        T, ORDN, tgt, ___, ___)             \
  V(JumpIfNeStrict_Wide,                   T, WIDE, tgt, ___, ___)             \
  V(JumpIfTrue,                            T, ORDN, tgt, ___, ___)             \
  V(JumpIfTrue_Wide,                       T, WIDE, tgt, ___, ___)             \
  V(JumpIfFalse,                           T, ORDN, tgt, ___, ___)             \
  V(JumpIfFalse_Wide,                      T, WIDE, tgt, ___, ___)             \
  V(JumpIfNull,                            T, ORDN, tgt, ___, ___)             \
  V(JumpIfNull_Wide,                       T, WIDE, tgt, ___, ___)             \
  V(JumpIfNotNull,                         T, ORDN, tgt, ___, ___)             \
  V(JumpIfNotNull_Wide,                    T, WIDE, tgt, ___, ___)             \
  V(DirectCall,                          D_F, ORDN, num, num, ___)             \
  V(DirectCall_Wide,                     D_F, WIDE, num, num, ___)             \
  V(Unused21,                              0, RESV, ___, ___, ___)             \
  V(Unused22,                              0, RESV, ___, ___, ___)             \
  V(InterfaceCall,                       D_F, ORDN, num, num, ___)             \
  V(InterfaceCall_Wide,                  D_F, WIDE, num, num, ___)             \
  V(Unused23,                              0, RESV, ___, ___, ___)             \
  V(Unused24,                              0, RESV, ___, ___, ___)             \
  V(Unused25,                              0, RESV, ___, ___, ___)             \
  V(Unused26,                              0, RESV, ___, ___, ___)             \
  V(Unused27,                              0, RESV, ___, ___, ___)             \
  V(Unused28,                              0, RESV, ___, ___, ___)             \
  V(UncheckedInterfaceCall,              D_F, ORDN, num, num, ___)             \
  V(UncheckedInterfaceCall_Wide,         D_F, WIDE, num, num, ___)             \
  V(DynamicCall,                         D_F, ORDN, num, num, ___)             \
  V(DynamicCall_Wide,                    D_F, WIDE, num, num, ___)             \
  V(NativeCall,                            D, ORDN, lit, ___, ___)             \
  V(NativeCall_Wide,                       D, WIDE, lit, ___, ___)             \
  V(ReturnTOS,                             0, ORDN, ___, ___, ___)             \
  V(Unused29,                              0, RESV, ___, ___, ___)             \
  V(AssertAssignable,                    A_E, ORDN, num, lit, ___)             \
  V(AssertAssignable_Wide,               A_E, WIDE, num, lit, ___)             \
  V(Unused30,                              0, RESV, ___, ___, ___)             \
  V(Unused31,                              0, RESV, ___, ___, ___)             \
  V(AssertBoolean,                         A, ORDN, num, ___, ___)             \
  V(AssertSubtype,                         0, ORDN, ___, ___, ___)             \
  V(LoadTypeArgumentsField,                D, ORDN, lit, ___, ___)             \
  V(LoadTypeArgumentsField_Wide,           D, WIDE, lit, ___, ___)             \
  V(InstantiateType,                       D, ORDN, lit, ___, ___)             \
  V(InstantiateType_Wide,                  D, WIDE, lit, ___, ___)             \
  V(InstantiateTypeArgumentsTOS,         A_E, ORDN, num, lit, ___)             \
  V(InstantiateTypeArgumentsTOS_Wide,    A_E, WIDE, num, lit, ___)             \
  V(Unused32,                              0, RESV, ___, ___, ___)             \
  V(Unused33,                              0, RESV, ___, ___, ___)             \
  V(Unused34,                              0, RESV, ___, ___, ___)             \
  V(Unused35,                              0, RESV, ___, ___, ___)             \
  V(Throw,                                 A, ORDN, num, ___, ___)             \
  V(SetFrame,                              A, ORDN, num, ___, num)             \
  V(MoveSpecial,                         A_Y, ORDN, num, xeg, ___)             \
  V(MoveSpecial_Wide,                    A_Y, WIDE, num, xeg, ___)             \
  V(BooleanNegateTOS,                      0, ORDN, ___, ___, ___)             \
  V(EqualsNull,                            0, ORDN, ___, ___, ___)             \
  V(Unused36,                              0, RESV, ___, ___, ___)             \
  V(Unused37,                              0, RESV, ___, ___, ___)             \
  V(NegateInt,                             0, ORDN, ___, ___, ___)             \
  V(AddInt,                                0, ORDN, ___, ___, ___)             \
  V(SubInt,                                0, ORDN, ___, ___, ___)             \
  V(MulInt,                                0, ORDN, ___, ___, ___)             \
  V(TruncDivInt,                           0, ORDN, ___, ___, ___)             \
  V(ModInt,                                0, ORDN, ___, ___, ___)             \
  V(BitAndInt,                             0, ORDN, ___, ___, ___)             \
  V(BitOrInt,                              0, ORDN, ___, ___, ___)             \
  V(BitXorInt,                             0, ORDN, ___, ___, ___)             \
  V(ShlInt,                                0, ORDN, ___, ___, ___)             \
  V(ShrInt,                                0, ORDN, ___, ___, ___)             \
  V(CompareIntEq,                          0, ORDN, ___, ___, ___)             \
  V(CompareIntGt,                          0, ORDN, ___, ___, ___)             \
  V(CompareIntLt,                          0, ORDN, ___, ___, ___)             \
  V(CompareIntGe,                          0, ORDN, ___, ___, ___)             \
  V(CompareIntLe,                          0, ORDN, ___, ___, ___)             \
  V(NegateDouble,                          0, ORDN, ___, ___, ___)             \
  V(AddDouble,                             0, ORDN, ___, ___, ___)             \
  V(SubDouble,                             0, ORDN, ___, ___, ___)             \
  V(MulDouble,                             0, ORDN, ___, ___, ___)             \
  V(DivDouble,                             0, ORDN, ___, ___, ___)             \
  V(CompareDoubleEq,                       0, ORDN, ___, ___, ___)             \
  V(CompareDoubleGt,                       0, ORDN, ___, ___, ___)             \
  V(CompareDoubleLt,                       0, ORDN, ___, ___, ___)             \
  V(CompareDoubleGe,                       0, ORDN, ___, ___, ___)             \
  V(CompareDoubleLe,                       0, ORDN, ___, ___, ___)             \

  // These bytecodes are only generated within the VM. Reassigning their
  // opcodes is not a breaking change.
#define INTERNAL_KERNEL_BYTECODES_LIST(V)                                      \
  V(VMInternal_ImplicitGetter,             0, ORDN, ___, ___, ___)             \
  V(VMInternal_ImplicitSetter,             0, ORDN, ___, ___, ___)             \
  V(VMInternal_ImplicitStaticGetter,       0, ORDN, ___, ___, ___)             \
  V(VMInternal_MethodExtractor,            0, ORDN, ___, ___, ___)             \
  V(VMInternal_InvokeClosure,              0, ORDN, ___, ___, ___)             \
  V(VMInternal_InvokeField,                0, ORDN, ___, ___, ___)             \
  V(VMInternal_ForwardDynamicInvocation,   0, ORDN, ___, ___, ___)             \
  V(VMInternal_NoSuchMethodDispatcher,     0, ORDN, ___, ___, ___)             \
  V(VMInternal_ImplicitStaticClosure,      0, ORDN, ___, ___, ___)             \
  V(VMInternal_ImplicitInstanceClosure,    0, ORDN, ___, ___, ___)             \

#define KERNEL_BYTECODES_LIST(V)                                               \
  PUBLIC_KERNEL_BYTECODES_LIST(V)                                              \
  INTERNAL_KERNEL_BYTECODES_LIST(V)

// clang-format on

typedef uint8_t KBCInstr;

class KernelBytecode {
 public:
  // Magic value of bytecode files.
  static const intptr_t kMagicValue = 0x44424332;  // 'DBC2'
  // Minimum bytecode format version supported by VM.
  static const intptr_t kMinSupportedBytecodeFormatVersion = 3;
  // Maximum bytecode format version supported by VM.
  // The range of supported versions should include version produced by bytecode
  // generator (currentBytecodeFormatVersion in pkg/vm/lib/bytecode/dbc.dart).
  static const intptr_t kMaxSupportedBytecodeFormatVersion = 9;

  enum Opcode {
#define DECLARE_BYTECODE(name, encoding, kind, op1, op2, op3) k##name,
    KERNEL_BYTECODES_LIST(DECLARE_BYTECODE)
#undef DECLARE_BYTECODE
  };

  static const char* NameOf(Opcode op) {
    const char* names[] = {
#define NAME(name, encoding, kind, op1, op2, op3) #name,
        KERNEL_BYTECODES_LIST(NAME)
#undef NAME
    };
    return names[op];
  }

  static const intptr_t kInstructionSize[];

  enum SpecialIndex {
    kExceptionSpecialIndex,
    kStackTraceSpecialIndex,
    kSpecialIndexCount
  };

 private:
  static const intptr_t kWideModifier = 1;

  static_assert(kMinSupportedBytecodeFormatVersion < 7,
                "Cleanup support for old bytecode format versions");
  DART_FORCE_INLINE static bool IsOld(const KBCInstr* instr) {
    return DecodeOpcode(instr) < kTrap;
  }

  // Should be used only on instructions with wide variants.
  DART_FORCE_INLINE static bool IsWide(const KBCInstr* instr) {
    return ((DecodeOpcode(instr) & kWideModifier) != 0);
  }

 public:
  DART_FORCE_INLINE static uint8_t DecodeA(const KBCInstr* bc) { return bc[1]; }

  DART_FORCE_INLINE static uint8_t DecodeB(const KBCInstr* bc) { return bc[2]; }

  DART_FORCE_INLINE static uint8_t DecodeC(const KBCInstr* bc) { return bc[3]; }

  DART_FORCE_INLINE static uint32_t DecodeD(const KBCInstr* bc) {
    if (IsOld(bc)) {
      return static_cast<uint16_t>(bc[2]) | (static_cast<uint16_t>(bc[3]) << 8);
    } else if (IsWide(bc)) {
      return static_cast<uint32_t>(bc[1]) |
             (static_cast<uint32_t>(bc[2]) << 8) |
             (static_cast<uint32_t>(bc[3]) << 16) |
             (static_cast<uint32_t>(bc[4]) << 24);
    } else {
      return bc[1];
    }
  }

  DART_FORCE_INLINE static int32_t DecodeX(const KBCInstr* bc) {
    if (IsOld(bc)) {
      return static_cast<int16_t>(static_cast<uint16_t>(bc[2]) |
                                  (static_cast<uint16_t>(bc[3]) << 8));
    } else if (IsWide(bc)) {
      return static_cast<int32_t>(static_cast<uint32_t>(bc[1]) |
                                  (static_cast<uint32_t>(bc[2]) << 8) |
                                  (static_cast<uint32_t>(bc[3]) << 16) |
                                  (static_cast<uint32_t>(bc[4]) << 24));
    } else {
      return static_cast<int8_t>(bc[1]);
    }
  }

  DART_FORCE_INLINE static int32_t DecodeT(const KBCInstr* bc) {
    if (IsOld(bc)) {
      return static_cast<int32_t>((static_cast<uint32_t>(bc[1]) << 8) |
                                  (static_cast<uint32_t>(bc[2]) << 16) |
                                  (static_cast<uint32_t>(bc[3]) << 24)) >>
             (8 - 2);
    } else if (IsWide(bc)) {
      return static_cast<int32_t>((static_cast<uint32_t>(bc[1]) << 8) |
                                  (static_cast<uint32_t>(bc[2]) << 16) |
                                  (static_cast<uint32_t>(bc[3]) << 24)) >>
             8;
    } else {
      return static_cast<int8_t>(bc[1]);
    }
  }

  DART_FORCE_INLINE static uint32_t DecodeE(const KBCInstr* bc) {
    if (IsOld(bc)) {
      return static_cast<uint16_t>(bc[2]) | (static_cast<uint16_t>(bc[3]) << 8);
    } else if (IsWide(bc)) {
      return static_cast<uint32_t>(bc[2]) |
             (static_cast<uint32_t>(bc[3]) << 8) |
             (static_cast<uint32_t>(bc[4]) << 16) |
             (static_cast<uint32_t>(bc[5]) << 24);
    } else {
      return bc[2];
    }
  }

  DART_FORCE_INLINE static int32_t DecodeY(const KBCInstr* bc) {
    if (IsOld(bc)) {
      return static_cast<int16_t>(static_cast<uint16_t>(bc[2]) |
                                  (static_cast<uint16_t>(bc[3]) << 8));
    } else if (IsWide(bc)) {
      return static_cast<int32_t>(static_cast<uint32_t>(bc[2]) |
                                  (static_cast<uint32_t>(bc[3]) << 8) |
                                  (static_cast<uint32_t>(bc[4]) << 16) |
                                  (static_cast<uint32_t>(bc[5]) << 24));
    } else {
      return static_cast<int8_t>(bc[2]);
    }
  }

  DART_FORCE_INLINE static uint8_t DecodeF(const KBCInstr* bc) {
    if (IsOld(bc)) {
      return bc[1];
    } else if (IsWide(bc)) {
      return bc[5];
    } else {
      return bc[2];
    }
  }

  DART_FORCE_INLINE static Opcode DecodeOpcode(const KBCInstr* bc) {
    return static_cast<Opcode>(bc[0]);
  }

  DART_FORCE_INLINE static const KBCInstr* Next(const KBCInstr* bc) {
    return bc + kInstructionSize[DecodeOpcode(bc)];
  }

  DART_FORCE_INLINE static bool IsJumpOpcode(const KBCInstr* instr) {
    switch (DecodeOpcode(instr)) {
      case KernelBytecode::kJump_Old:
      case KernelBytecode::kJumpIfNoAsserts_Old:
      case KernelBytecode::kJumpIfNotZeroTypeArgs_Old:
      case KernelBytecode::kJumpIfEqStrict_Old:
      case KernelBytecode::kJumpIfNeStrict_Old:
      case KernelBytecode::kJumpIfTrue_Old:
      case KernelBytecode::kJumpIfFalse_Old:
      case KernelBytecode::kJumpIfNull_Old:
      case KernelBytecode::kJumpIfNotNull_Old:
      case KernelBytecode::kJump:
      case KernelBytecode::kJump_Wide:
      case KernelBytecode::kJumpIfNoAsserts:
      case KernelBytecode::kJumpIfNoAsserts_Wide:
      case KernelBytecode::kJumpIfNotZeroTypeArgs:
      case KernelBytecode::kJumpIfNotZeroTypeArgs_Wide:
      case KernelBytecode::kJumpIfEqStrict:
      case KernelBytecode::kJumpIfEqStrict_Wide:
      case KernelBytecode::kJumpIfNeStrict:
      case KernelBytecode::kJumpIfNeStrict_Wide:
      case KernelBytecode::kJumpIfTrue:
      case KernelBytecode::kJumpIfTrue_Wide:
      case KernelBytecode::kJumpIfFalse:
      case KernelBytecode::kJumpIfFalse_Wide:
      case KernelBytecode::kJumpIfNull:
      case KernelBytecode::kJumpIfNull_Wide:
      case KernelBytecode::kJumpIfNotNull:
      case KernelBytecode::kJumpIfNotNull_Wide:
        return true;

      default:
        return false;
    }
  }

  DART_FORCE_INLINE static bool IsLoadConstantOpcode(const KBCInstr* instr) {
    switch (DecodeOpcode(instr)) {
      case KernelBytecode::kLoadConstant:
      case KernelBytecode::kLoadConstant_Wide:
      case KernelBytecode::kLoadConstant_Old:
        return true;
      default:
        return false;
    }
  }

  DART_FORCE_INLINE static bool IsCheckStackOpcode(const KBCInstr* instr) {
    switch (DecodeOpcode(instr)) {
      case KernelBytecode::kCheckStack:
      case KernelBytecode::kCheckStack_Old:
        return true;
      default:
        return false;
    }
  }

  DART_FORCE_INLINE static bool IsEntryOptionalOpcode(const KBCInstr* instr) {
    switch (DecodeOpcode(instr)) {
      case KernelBytecode::kEntryOptional:
      case KernelBytecode::kEntryOptional_Old:
        return true;
      default:
        return false;
    }
  }

  DART_FORCE_INLINE static bool IsFrameOpcode(const KBCInstr* instr) {
    switch (DecodeOpcode(instr)) {
      case KernelBytecode::kFrame:
      case KernelBytecode::kFrame_Wide:
      case KernelBytecode::kFrame_Old:
        return true;
      default:
        return false;
    }
  }

  DART_FORCE_INLINE static bool IsSetFrameOpcode(const KBCInstr* instr) {
    switch (DecodeOpcode(instr)) {
      case KernelBytecode::kSetFrame:
      case KernelBytecode::kSetFrame_Old:
        return true;
      default:
        return false;
    }
  }

  DART_FORCE_INLINE static bool IsCallOpcode_Old(const KBCInstr* instr) {
    switch (DecodeOpcode(instr)) {
      case KernelBytecode::kDirectCall_Old:
      case KernelBytecode::kInterfaceCall_Old:
      case KernelBytecode::kUncheckedInterfaceCall_Old:
      case KernelBytecode::kDynamicCall_Old:
        return true;

      default:
        return false;
    }
  }

  DART_FORCE_INLINE static bool IsNativeCallOpcode(const KBCInstr* instr) {
    switch (DecodeOpcode(instr)) {
      case KernelBytecode::kNativeCall:
      case KernelBytecode::kNativeCall_Wide:
      case KernelBytecode::kNativeCall_Old:
        return true;
      default:
        return false;
    }
  }

  // The interpreter and this function must agree on the opcodes.
  DART_FORCE_INLINE static bool IsDebugBreakCheckedOpcode(
      const KBCInstr* instr) {
    switch (DecodeOpcode(instr)) {
      case KernelBytecode::kCheckStack:
      case KernelBytecode::kDirectCall:
      case KernelBytecode::kInterfaceCall:
      case KernelBytecode::kUncheckedInterfaceCall:
      case KernelBytecode::kDynamicCall:
      case KernelBytecode::kReturnTOS:
        return true;
      default:
        return false;
    }
  }

  static const uint8_t kNativeCallToGrowableListArgc = 2;

  DART_FORCE_INLINE static uint8_t DecodeArgc_Old(const KBCInstr* ret_addr) {
    const intptr_t kOldInstructionSize = 4;
    const KBCInstr* call = ret_addr - kOldInstructionSize;
    ASSERT(IsOld(call));
    if (DecodeOpcode(call) == KernelBytecode::kNativeCall_Old) {
      // The only NativeCall redirecting to a bytecode function is the call
      // to new _GrowableList<E>(0).
      return kNativeCallToGrowableListArgc;
    }
    ASSERT(IsCallOpcode_Old(call));
    return DecodeA(call);
  }

  // Returns a fake return address which points after the 2-argument
  // bytecode call, followed by ReturnTOS instructions.
  static const KBCInstr* GetNativeCallToGrowableListReturnTrampoline();

  DART_FORCE_INLINE static uint8_t DecodeArgc(const KBCInstr* ret_addr) {
    // All call instructions have DF encoding, with argc being the last byte
    // regardless of whether the wide variant is used or not.
    return ret_addr[-1];
  }

  // Converts bytecode PC into an offset.
  // For return addresses used in PcDescriptors, PC is also augmented by 1.
  // TODO(regis): Eliminate this correction.
  static intptr_t BytecodePcToOffset(uint32_t pc, bool is_return_address) {
    return pc + (is_return_address ? 1 : 0);
  }

  static uint32_t OffsetToBytecodePc(intptr_t offset, bool is_return_address) {
    return offset - (is_return_address ? 1 : 0);
  }

  static void GetVMInternalBytecodeInstructions(Opcode opcode,
                                                const KBCInstr** instructions,
                                                intptr_t* instructions_size);

 private:
  DISALLOW_ALLOCATION();
  DISALLOW_IMPLICIT_CONSTRUCTORS(KernelBytecode);
};

}  // namespace dart

#endif  // RUNTIME_VM_CONSTANTS_KBC_H_

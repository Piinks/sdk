// Copyright (c) 2018, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

library vm.bytecode.assembler;

import 'package:kernel/ast.dart' show TreeNode;

import 'dbc.dart';
import 'exceptions.dart' show ExceptionsTable;
import 'local_variable_table.dart' show LocalVariableTable;
import 'source_positions.dart' show SourcePositions;

class Label {
  final bool allowsBackwardJumps;
  List<int> _jumps = <int>[];
  int offset = -1;

  Label({this.allowsBackwardJumps: false});

  bool get isBound => offset >= 0;

  int jumpOperand(int jumpOffset) {
    if (isBound) {
      if (offset <= jumpOffset && !allowsBackwardJumps) {
        throw 'Backward jump to this label is not allowed';
      }
      // Jump instruction takes a relative offset.
      return offset - jumpOffset;
    }
    _jumps.add(jumpOffset);
    return 0;
  }

  List<int> bind(int offset) {
    assert(!isBound);
    this.offset = offset;
    final jumps = _jumps;
    _jumps = null;
    return jumps;
  }
}

class BytecodeAssembler {
  static const int kByteMask = 0xFF;
  static const int kUint32Mask = 0xFFFFFFFF;
  static const int kMinInt8 = -0x80;
  static const int kMaxInt8 = 0x7F;
  static const int kMinInt24 = -0x800000;
  static const int kMaxInt24 = 0x7FFFFF;
  static const int kMinInt32 = -0x80000000;
  static const int kMaxInt32 = 0x7FFFFFFF;

  // TODO(alexmarkov): figure out more efficient storage for generated bytecode.
  final List<int> bytecode = new List<int>();
  final ExceptionsTable exceptionsTable = new ExceptionsTable();
  final LocalVariableTable localVariableTable = new LocalVariableTable();
  final SourcePositions sourcePositions = new SourcePositions();
  bool isUnreachable = false;
  int currentSourcePosition = TreeNode.noOffset;

  BytecodeAssembler();

  int get offset => bytecode.length;

  void bind(Label label) {
    final List<int> jumps = label.bind(offset);
    for (int jumpOffset in jumps) {
      _patchJump(jumpOffset, label.jumpOperand(jumpOffset));
    }
    if (jumps.isNotEmpty || label.allowsBackwardJumps) {
      isUnreachable = false;
    }
  }

  void emitSourcePosition() {
    if (currentSourcePosition != TreeNode.noOffset && !isUnreachable) {
      sourcePositions.add(offset, currentSourcePosition);
    }
  }

  void _emitByte(int abyte) {
    assert(_isUint8(abyte));
    bytecode.add(abyte);
  }

  void _emitBytes2(int b0, int b1) {
    assert(_isUint8(b0) && _isUint8(b1));
    bytecode.add(b0);
    bytecode.add(b1);
  }

  void _emitBytes3(int b0, int b1, int b2) {
    assert(_isUint8(b0) && _isUint8(b1) && _isUint8(b2));
    bytecode.add(b0);
    bytecode.add(b1);
    bytecode.add(b2);
  }

  void _emitBytes4(int b0, int b1, int b2, int b3) {
    assert(_isUint8(b0) && _isUint8(b1) && _isUint8(b2) && _isUint8(b3));
    bytecode.add(b0);
    bytecode.add(b1);
    bytecode.add(b2);
    bytecode.add(b3);
  }

  void _emitBytes5(int b0, int b1, int b2, int b3, int b4) {
    assert(_isUint8(b0) &&
        _isUint8(b1) &&
        _isUint8(b2) &&
        _isUint8(b3) &&
        _isUint8(b4));
    bytecode.add(b0);
    bytecode.add(b1);
    bytecode.add(b2);
    bytecode.add(b3);
    bytecode.add(b4);
  }

  void _emitBytes6(int b0, int b1, int b2, int b3, int b4, int b5) {
    assert(_isUint8(b0) &&
        _isUint8(b1) &&
        _isUint8(b2) &&
        _isUint8(b3) &&
        _isUint8(b4) &&
        _isUint8(b5));
    bytecode.add(b0);
    bytecode.add(b1);
    bytecode.add(b2);
    bytecode.add(b3);
    bytecode.add(b4);
    bytecode.add(b5);
  }

  int _byteAt(int pos) {
    return bytecode[pos];
  }

  void _setByteAt(int pos, int value) {
    assert(_isUint8(value));
    bytecode[pos] = value;
  }

  int _byte0(int v) => v & kByteMask;
  int _byte1(int v) => (v >> 8) & kByteMask;
  int _byte2(int v) => (v >> 16) & kByteMask;
  int _byte3(int v) => (v >> 24) & kByteMask;

  bool _isInt8(int v) => (kMinInt8 <= v) && (v <= kMaxInt8);
  bool _isInt24(int v) => (kMinInt24 <= v) && (v <= kMaxInt24);
  bool _isInt32(int v) => (kMinInt32 <= v) && (v <= kMaxInt32);
  bool _isUint8(int v) => (v & kByteMask) == v;
  bool _isUint32(int v) => (v & kUint32Mask) == v;

  void _emitInstruction0(Opcode opcode) {
    if (isUnreachable) {
      return;
    }
    _emitByte(opcode.index);
  }

  void _emitInstructionA(Opcode opcode, int ra) {
    if (isUnreachable) {
      return;
    }
    _emitBytes2(opcode.index, ra);
  }

  void _emitInstructionD(Opcode opcode, int rd) {
    if (isUnreachable) {
      return;
    }
    if (_isUint8(rd)) {
      _emitBytes2(opcode.index, rd);
    } else {
      assert(_isUint32(rd));
      _emitBytes5(opcode.index + kWideModifier, _byte0(rd), _byte1(rd),
          _byte2(rd), _byte3(rd));
    }
  }

  void _emitInstructionX(Opcode opcode, int rx) {
    if (isUnreachable) {
      return;
    }
    if (_isInt8(rx)) {
      _emitBytes2(opcode.index, rx & kByteMask);
    } else {
      assert(_isInt32(rx));
      _emitBytes5(opcode.index + kWideModifier, _byte0(rx), _byte1(rx),
          _byte2(rx), _byte3(rx));
    }
  }

  void _emitInstructionAE(Opcode opcode, int ra, int re) {
    if (isUnreachable) {
      return;
    }
    if (_isUint8(re)) {
      _emitBytes3(opcode.index, ra, re);
    } else {
      assert(_isUint32(re));
      _emitBytes6(opcode.index + kWideModifier, ra, _byte0(re), _byte1(re),
          _byte2(re), _byte3(re));
    }
  }

  void _emitInstructionAY(Opcode opcode, int ra, int ry) {
    if (isUnreachable) {
      return;
    }
    if (_isInt8(ry)) {
      _emitBytes3(opcode.index, ra, ry & kByteMask);
    } else {
      assert(_isInt32(ry));
      _emitBytes6(opcode.index + kWideModifier, ra, _byte0(ry), _byte1(ry),
          _byte2(ry), _byte3(ry));
    }
  }

  void _emitInstructionDF(Opcode opcode, int rd, int rf) {
    if (isUnreachable) {
      return;
    }
    if (_isUint8(rd)) {
      _emitBytes3(opcode.index, rd, rf);
    } else {
      assert(_isUint32(rd));
      _emitBytes6(opcode.index + kWideModifier, _byte0(rd), _byte1(rd),
          _byte2(rd), _byte3(rd), rf);
    }
  }

  void _emitInstructionABC(Opcode opcode, int ra, int rb, int rc) {
    if (isUnreachable) {
      return;
    }
    _emitBytes4(opcode.index, ra, rb, rc);
  }

  void emitSpecializedBytecode(Opcode opcode) {
    assert(BytecodeFormats[opcode].encoding == Encoding.k0);
    emitSourcePosition();
    _emitInstruction0(opcode);
  }

  void _emitJumpInstruction(Opcode opcode, Label label) {
    assert(isJump(opcode));
    if (isUnreachable) {
      return;
    }
    final int target = label.jumpOperand(offset);
    // Use compact representation only for backwards jumps.
    // TODO(alexmarkov): generate compact forward jumps as well.
    if (label.isBound && _isInt8(target)) {
      _emitBytes2(opcode.index, target & kByteMask);
    } else {
      assert(_isInt24(target));
      _emitBytes4(opcode.index + kWideModifier, _byte0(target), _byte1(target),
          _byte2(target));
    }
  }

  void _patchJump(int pos, int rt) {
    final Opcode opcode = Opcode.values[_byteAt(pos) - kWideModifier];
    assert(hasWideVariant(opcode));
    assert(isJump(opcode));
    assert(_isInt24(rt));
    _setByteAt(pos + 1, _byte0(rt));
    _setByteAt(pos + 2, _byte1(rt));
    _setByteAt(pos + 3, _byte2(rt));
  }

  void emitTrap() {
    _emitInstruction0(Opcode.kTrap);
    isUnreachable = true;
  }

  void emitDrop1() {
    _emitInstruction0(Opcode.kDrop1);
  }

  void emitJump(Label label) {
    emitSourcePosition();
    _emitJumpInstruction(Opcode.kJump, label);
    isUnreachable = true;
  }

  void emitJumpIfNoAsserts(Label label) {
    _emitJumpInstruction(Opcode.kJumpIfNoAsserts, label);
  }

  void emitJumpIfNotZeroTypeArgs(Label label) {
    _emitJumpInstruction(Opcode.kJumpIfNotZeroTypeArgs, label);
  }

  void emitJumpIfEqStrict(Label label) {
    _emitJumpInstruction(Opcode.kJumpIfEqStrict, label);
  }

  void emitJumpIfNeStrict(Label label) {
    _emitJumpInstruction(Opcode.kJumpIfNeStrict, label);
  }

  void emitJumpIfTrue(Label label) {
    _emitJumpInstruction(Opcode.kJumpIfTrue, label);
  }

  void emitJumpIfFalse(Label label) {
    _emitJumpInstruction(Opcode.kJumpIfFalse, label);
  }

  void emitJumpIfNull(Label label) {
    _emitJumpInstruction(Opcode.kJumpIfNull, label);
  }

  void emitJumpIfNotNull(Label label) {
    _emitJumpInstruction(Opcode.kJumpIfNotNull, label);
  }

  void emitReturnTOS() {
    emitSourcePosition();
    _emitInstruction0(Opcode.kReturnTOS);
    isUnreachable = true;
  }

  void emitPush(int rx) {
    _emitInstructionX(Opcode.kPush, rx);
  }

  void emitLoadConstant(int ra, int re) {
    _emitInstructionAE(Opcode.kLoadConstant, ra, re);
  }

  void emitPushConstant(int rd) {
    _emitInstructionD(Opcode.kPushConstant, rd);
  }

  void emitPushNull() {
    _emitInstruction0(Opcode.kPushNull);
  }

  void emitPushTrue() {
    _emitInstruction0(Opcode.kPushTrue);
  }

  void emitPushFalse() {
    _emitInstruction0(Opcode.kPushFalse);
  }

  void emitPushInt(int rx) {
    _emitInstructionX(Opcode.kPushInt, rx);
  }

  void emitStoreLocal(int rx) {
    emitSourcePosition();
    _emitInstructionX(Opcode.kStoreLocal, rx);
  }

  void emitPopLocal(int rx) {
    emitSourcePosition();
    _emitInstructionX(Opcode.kPopLocal, rx);
  }

  void emitDirectCall(int rd, int rf) {
    emitSourcePosition();
    _emitInstructionDF(Opcode.kDirectCall, rd, rf);
  }

  void emitInterfaceCall(int rd, int rf) {
    emitSourcePosition();
    _emitInstructionDF(Opcode.kInterfaceCall, rd, rf);
  }

  void emitUncheckedInterfaceCall(int rd, int rf) {
    emitSourcePosition();
    _emitInstructionDF(Opcode.kUncheckedInterfaceCall, rd, rf);
  }

  void emitDynamicCall(int rd, int rf) {
    emitSourcePosition();
    _emitInstructionDF(Opcode.kDynamicCall, rd, rf);
  }

  void emitNativeCall(int rd) {
    emitSourcePosition();
    _emitInstructionD(Opcode.kNativeCall, rd);
  }

  void emitStoreStaticTOS(int rd) {
    emitSourcePosition();
    _emitInstructionD(Opcode.kStoreStaticTOS, rd);
  }

  void emitPushStatic(int rd) {
    _emitInstructionD(Opcode.kPushStatic, rd);
  }

  void emitCreateArrayTOS() {
    _emitInstruction0(Opcode.kCreateArrayTOS);
  }

  void emitAllocate(int rd) {
    emitSourcePosition();
    _emitInstructionD(Opcode.kAllocate, rd);
  }

  void emitAllocateT() {
    emitSourcePosition();
    _emitInstruction0(Opcode.kAllocateT);
  }

  void emitStoreIndexedTOS() {
    _emitInstruction0(Opcode.kStoreIndexedTOS);
  }

  void emitStoreFieldTOS(int rd) {
    emitSourcePosition();
    _emitInstructionD(Opcode.kStoreFieldTOS, rd);
  }

  void emitStoreContextParent() {
    _emitInstruction0(Opcode.kStoreContextParent);
  }

  void emitStoreContextVar(int ra, int re) {
    _emitInstructionAE(Opcode.kStoreContextVar, ra, re);
  }

  void emitLoadFieldTOS(int rd) {
    _emitInstructionD(Opcode.kLoadFieldTOS, rd);
  }

  void emitLoadTypeArgumentsField(int rd) {
    _emitInstructionD(Opcode.kLoadTypeArgumentsField, rd);
  }

  void emitLoadContextParent() {
    _emitInstruction0(Opcode.kLoadContextParent);
  }

  void emitLoadContextVar(int ra, int re) {
    _emitInstructionAE(Opcode.kLoadContextVar, ra, re);
  }

  void emitBooleanNegateTOS() {
    _emitInstruction0(Opcode.kBooleanNegateTOS);
  }

  void emitThrow(int ra) {
    emitSourcePosition();
    _emitInstructionA(Opcode.kThrow, ra);
    isUnreachable = true;
  }

  void emitEntry(int rd) {
    _emitInstructionD(Opcode.kEntry, rd);
  }

  void emitFrame(int rd) {
    _emitInstructionD(Opcode.kFrame, rd);
  }

  void emitSetFrame(int ra) {
    _emitInstructionA(Opcode.kSetFrame, ra);
  }

  void emitAllocateContext(int ra, int re) {
    _emitInstructionAE(Opcode.kAllocateContext, ra, re);
  }

  void emitCloneContext(int ra, int re) {
    _emitInstructionAE(Opcode.kCloneContext, ra, re);
  }

  void emitMoveSpecial(SpecialIndex ra, int ry) {
    _emitInstructionAY(Opcode.kMoveSpecial, ra.index, ry);
  }

  void emitInstantiateType(int rd) {
    emitSourcePosition();
    _emitInstructionD(Opcode.kInstantiateType, rd);
  }

  void emitInstantiateTypeArgumentsTOS(int ra, int re) {
    emitSourcePosition();
    _emitInstructionAE(Opcode.kInstantiateTypeArgumentsTOS, ra, re);
  }

  void emitAssertAssignable(int ra, int re) {
    emitSourcePosition();
    _emitInstructionAE(Opcode.kAssertAssignable, ra, re);
  }

  void emitAssertSubtype() {
    emitSourcePosition();
    _emitInstruction0(Opcode.kAssertSubtype);
  }

  void emitAssertBoolean(int ra) {
    emitSourcePosition();
    _emitInstructionA(Opcode.kAssertBoolean, ra);
  }

  void emitCheckStack(int ra) {
    emitSourcePosition();
    _emitInstructionA(Opcode.kCheckStack, ra);
  }

  void emitCheckFunctionTypeArgs(int ra, int re) {
    emitSourcePosition();
    _emitInstructionAE(Opcode.kCheckFunctionTypeArgs, ra, re);
  }

  void emitEntryFixed(int ra, int re) {
    _emitInstructionAE(Opcode.kEntryFixed, ra, re);
  }

  void emitEntryOptional(int ra, int rb, int rc) {
    _emitInstructionABC(Opcode.kEntryOptional, ra, rb, rc);
  }

  void emitAllocateClosure(int rd) {
    emitSourcePosition();
    _emitInstructionD(Opcode.kAllocateClosure, rd);
  }
}

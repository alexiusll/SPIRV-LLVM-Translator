//===- SPIRVStream.h - Class to represent a SPIR-V Stream -------*- C++ -*-===//
//
//                     The LLVM/SPIRV Translator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// Copyright (c) 2014 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal with the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimers.
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimers in the documentation
// and/or other materials provided with the distribution.
// Neither the names of Advanced Micro Devices, Inc., nor the names of its
// contributors may be used to endorse or promote products derived from this
// Software without specific prior written permission.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
// THE SOFTWARE.
//
//===----------------------------------------------------------------------===//
/// \file
///
/// This file defines Word class for SPIR-V.
///
//===----------------------------------------------------------------------===//

#ifndef SPIRV_LIBSPIRV_SPIRVSTREAM_H
#define SPIRV_LIBSPIRV_SPIRVSTREAM_H

#include "SPIRVDebug.h"
#include "SPIRVExtInst.h"
#include "SPIRVModule.h"
#include <cctype>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace SPIRV {

#ifndef _SPIRV_SUPPORT_TEXT_FMT
#define _SPIRV_SUPPORT_TEXT_FMT
#endif

#ifdef _SPIRV_SUPPORT_TEXT_FMT
// Use textual format for SPIRV.
extern bool SPIRVUseTextFormat;
#endif

class SPIRVFunction;
class SPIRVBasicBlock;

class SPIRVDecoder {
public:
  SPIRVDecoder(std::istream &InputStream, SPIRVModule &Module)
      : IS(InputStream), M(Module), WordCount(0), OpCode(OpNop), Scope(NULL) {}
  SPIRVDecoder(std::istream &InputStream, SPIRVFunction &F);
  SPIRVDecoder(std::istream &InputStream, SPIRVBasicBlock &BB);

  void setScope(SPIRVEntry *);
  bool getWordCountAndOpCode();
  SPIRVEntry *getEntry();
  void validate() const;
  void ignore(size_t N);
  void ignoreInstruction();
  std::vector<SPIRVEntry *>
  getContinuedInstructions(const spv::Op ContinuedOpCode);

  std::istream &IS;
  SPIRVModule &M;
  SPIRVWord WordCount;
  Op OpCode;
  SPIRVEntry *Scope; // A function or basic block
};

class SPIRVEncoder {
public:
  explicit SPIRVEncoder(spv_ostream &OutputStream) : OS(OutputStream) {}
  spv_ostream &OS;
};

/// Output a new line in text mode. Do nothing in binary mode.
class SPIRVNL {
  friend spv_ostream &operator<<(spv_ostream &O, const SPIRVNL &E);
};

template <typename T>
const SPIRVDecoder &decodeBinary(const SPIRVDecoder &I, T &V) {
  uint32_t W;
  I.IS.read(reinterpret_cast<char *>(&W), sizeof(W));
  V = static_cast<T>(W);
  SPIRVDBG(spvdbgs() << "Read word: W = " << W << " V = " << V << '\n');
  return I;
}

#ifdef _SPIRV_SUPPORT_TEXT_FMT
///* 跳过spirv开头的 注释和空白。注释以';'开头，以'\n'结尾。
inline std::istream &skipcomment(std::istream &IS) {
  if (IS.eof() || IS.bad()) //* eof()：这是 std::istream 类的成员函数，用于检查是否已到达文件末尾。
    return IS; //* bad()：这是 std::istream 类的另一个成员函数，用于检查输入流是否处于错误状态，例如文件读取错误等。

  char C = IS.peek(); //* IS.peek()：peek() 是 std::istream 类的成员函数，用于预览输入流中的下一个字符，而不会将它从输入流中移除。
  //* 这意味着，调用 peek() 后，输入流的位置不会改变。

  //* std::char_traits 是 C++ 标准库中用于操作字符类型特性的模板类。
  //* not_eof 是 std::char_traits 类的静态函数，用于检查字符是否不等于 EOF（End of File）值。
  while (std::char_traits<char>::not_eof(C) && std::isspace(C)) {//* 在这里，它用于检查字符变量 C 是否不等于 EOF，即是否是有效字符。
    IS.get(); //* IS.get()：get() 是 std::istream 类的成员函数，用于从输入流中提取一个字符并将流指针前进。在这里，它被调用来读取当前空白字符，以便跳过它。
    C = IS.peek(); //* C = IS.peek()：这行代码类似于之前解释的部分，从输入流预览下一个字符，并将其赋值给字符变量 C。
  }
  
  while (std::char_traits<char>::not_eof(C) && C == ';') {
    //* ignore() 是 std::istream 类的成员函数，用于忽略输入流中的字符，直到遇到指定的终止字符（此处为换行符 \n）。
    //* std::numeric_limits<std::streamsize>::max() 表示最大的流大小值，这里表示要忽略直到换行符为止。
    IS.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    C = IS.peek();
    while (std::char_traits<char>::not_eof(C) && std::isspace(C)) {
      IS.get();
      C = IS.peek();
    }
  }

  return IS;
}
#endif

template <typename T>
const SPIRVDecoder &operator>>(const SPIRVDecoder &I, T &V) {
#ifdef _SPIRV_SUPPORT_TEXT_FMT
  if (SPIRVUseTextFormat) {
    uint32_t W;
    //*skipcomment: 跳过spirv开头的 注释和空白。注释以';'开头，以'\n'结尾。
    I.IS >> skipcomment >> W; 
    V = static_cast<T>(W);
    SPIRVDBG(spvdbgs() << "Read word: W = " << W << " V = " << V << '\n');
    return I;
  }
#endif
  return decodeBinary(I, V);
}

template <typename T>
const SPIRVDecoder &operator>>(const SPIRVDecoder &I, T *&P) {
  SPIRVId Id;
  I >> Id;
  P = static_cast<T *>(I.M.getEntry(Id));
  return I;
}

template <typename IterTy>
const SPIRVDecoder &operator>>(const SPIRVDecoder &Decoder,
                               const std::pair<IterTy, IterTy> &Range) {
  for (IterTy I = Range.first, E = Range.second; I != E; ++I)
    Decoder >> *I;
  return Decoder;
}

template <typename T>
const SPIRVDecoder &operator>>(const SPIRVDecoder &I, std::vector<T> &V) {
  for (size_t J = 0, E = V.size(); J != E; ++J)
    I >> V[J];
  return I;
}

template <typename T>
const SPIRVDecoder &operator>>(const SPIRVDecoder &I, std::optional<T> &V) {
  if (V)
    I >> V.value();
  return I;
}

template <typename T>
const SPIRVEncoder &operator<<(const SPIRVEncoder &O, T V) {
#ifdef _SPIRV_SUPPORT_TEXT_FMT
  if (SPIRVUseTextFormat) {
    O.OS << V << " ";
    return O;
  }
#endif
  uint32_t W = static_cast<uint32_t>(V);
  O.OS.write(reinterpret_cast<char *>(&W), sizeof(W));
  return O;
}

template <typename T>
const SPIRVEncoder &operator<<(const SPIRVEncoder &O, T *P) {
  return O << P->getId();
}
template <> const SPIRVEncoder &operator<<(const SPIRVEncoder &O, SPIRVType *P);

template <typename T>
const SPIRVEncoder &operator<<(const SPIRVEncoder &O, const std::vector<T> &V) {
  for (size_t I = 0, E = V.size(); I != E; ++I)
    O << V[I];
  return O;
}

template <typename T>
const SPIRVEncoder &operator<<(const SPIRVEncoder &O,
                               const std::optional<T> &V) {
  if (V)
    O << V.value();
  return O;
}

template <typename IterTy>
const SPIRVEncoder &operator<<(const SPIRVEncoder &Encoder,
                               const std::pair<IterTy, IterTy> &Range) {
  for (IterTy I = Range.first, E = Range.second; I != E; ++I)
    Encoder << *I;
  return Encoder;
}

#define SPIRV_DEC_ENCDEC(Type)                                                 \
  const SPIRVEncoder &operator<<(const SPIRVEncoder &O, Type V);               \
  const SPIRVDecoder &operator>>(const SPIRVDecoder &I, Type &V);

SPIRV_DEC_ENCDEC(Op)
SPIRV_DEC_ENCDEC(Capability)
SPIRV_DEC_ENCDEC(Decoration)
SPIRV_DEC_ENCDEC(OCLExtOpKind)
SPIRV_DEC_ENCDEC(SPIRVDebugExtOpKind)
SPIRV_DEC_ENCDEC(LinkageType)

const SPIRVEncoder &operator<<(const SPIRVEncoder &O, const std::string &Str);
const SPIRVDecoder &operator>>(const SPIRVDecoder &I, std::string &Str);

} // namespace SPIRV
#endif // SPIRV_LIBSPIRV_SPIRVSTREAM_H

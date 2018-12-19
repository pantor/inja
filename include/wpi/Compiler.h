//===-- llvm/Support/Compiler.h - Compiler abstraction support --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines several macros, based on the current compiler.  This allows
// use of compiler-specific features in a way that remains portable.
//
//===----------------------------------------------------------------------===//

#ifndef WPIUTIL_WPI_COMPILER_H
#define WPIUTIL_WPI_COMPILER_H

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif

#ifndef LLVM_LIKELY
#if __has_builtin(__builtin_expect)
#define LLVM_UNLIKELY(EXPR) __builtin_expect((bool)(EXPR), false)
#else
#define LLVM_UNLIKELY(EXPR) (EXPR)
#endif
#endif

#endif

/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INSTRTYPES_H
#define IGCLLVM_IR_INSTRTYPES_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/InstrTypes.h"
#if LLVM_VERSION_MAJOR >= 8
#include "llvm/IR/PatternMatch.h"
#endif

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR <= 7
    using llvm::TerminatorInst;
#else
    using TerminatorInst = llvm::Instruction;
#endif

    namespace BinaryOperator
    {
        inline bool isNot(const llvm::Value *V)
        {
#if LLVM_VERSION_MAJOR <= 7
            return llvm::BinaryOperator::isNot(V);
#else
            return llvm::PatternMatch::match(V, llvm::PatternMatch::m_Not(llvm::PatternMatch::m_Value()));
#endif
        }
    }
}

#endif

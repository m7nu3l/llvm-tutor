//==============================================================================
// FILE:
//    Profiler.h
//
// DESCRIPTION:
//    Declares the Profiler pass for the new and the legacy pass managers.
//
// License: MIT
//==============================================================================
#ifndef LLVM_CHALLENGE_PROFILER_H
#define LLVM_CHALLENGE_PROFILER_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

//------------------------------------------------------------------------------
// New PM interface
//------------------------------------------------------------------------------
struct Profiler : public llvm::PassInfoMixin<Profiler> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  bool runOnBasicBlock(llvm::BasicBlock &B);
  bool runOnFunction(llvm::Function &F);

private:
  llvm::Function *trampoline = nullptr;
};

//------------------------------------------------------------------------------
// Legacy PM interface
//------------------------------------------------------------------------------
struct LegacyProfiler : public llvm::FunctionPass {
  static char ID;
  LegacyProfiler() : FunctionPass(ID) {}
  bool runOnFunction(llvm::Function &F) override;

  Profiler Impl;
};

#endif

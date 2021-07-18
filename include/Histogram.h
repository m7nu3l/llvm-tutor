//==============================================================================
// FILE:
//    Histogram.h
//
// DESCRIPTION:
//    Declares the Histogram pass for the new and the legacy pass managers.
//
// License: MIT
//==============================================================================
#ifndef LLVM_CHALLENGE_HISTOGRAM_H
#define LLVM_CHALLENGE_HISTOGRAM_H

#include <list>
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

//------------------------------------------------------------------------------
// New PM interface
//------------------------------------------------------------------------------
struct Histogram : public llvm::PassInfoMixin<Histogram> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  bool runOnFunction(llvm::Function &F);

private:
  bool runOnBasicBlock(llvm::BasicBlock &B);
  void logBasicBlockSize(const size_t &size);
  std::list<size_t> sizes;
};

//------------------------------------------------------------------------------
// Legacy PM interface
//------------------------------------------------------------------------------
struct LegacyHistogram : public llvm::FunctionPass {
  static char ID;
  LegacyHistogram() : FunctionPass(ID) {}
  bool runOnFunction(llvm::Function &F) override;

  Histogram Impl;
};

#endif

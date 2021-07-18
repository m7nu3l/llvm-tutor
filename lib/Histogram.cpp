/*
  Task:
    a) Generate a histogram of the number of instructions in a basic
      block. For example, if there are four basic blocks with 3, 3, 5,
      and respectively 7 instructions, you should generate a histogram
      like this:

        *
        *  *  *
        3  5  7
*/

#include "Histogram.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <fstream>

using namespace llvm;

// Pass Option declaration
static cl::opt<std::string, false, llvm::cl::parser<std::string>>
    HistogramOutputFile{
        "histogram-output-file",
        cl::desc("Output file where basic block sizes are appended to. By "
                 "default, it is pwd/histogram.data"),
        cl::value_desc("output file"), cl::init("histogram.data"), cl::Optional};

//-----------------------------------------------------------------------------
// Histogram Implementation
//-----------------------------------------------------------------------------

bool Histogram::runOnFunction(Function &F) {

  llvm::errs() << "[Histogram Pass] Analyzing function: " << F.getName() << "\n";

  bool Changed = false;
  for (auto &BB : F) {
    Changed |= runOnBasicBlock(BB);
  }

  std::filebuf FileBuffer;

  // Append if it already exists, otherwise create a new one.
  // Appending is useful to process whole projects.
  if (!FileBuffer.open(HistogramOutputFile, std::ios_base::out |
                                                std::ios_base::in |
                                                std::ios_base::app)) {
    llvm::errs() << "Output file cannot be created\n";
    std::exit(-1);
  }

  std::ostream OutputStream(&FileBuffer);

  for (auto It = sizes.begin(); It != sizes.end(); It++) {
    OutputStream << *It << "\n";
  }

  FileBuffer.close();

  return Changed;
}

bool Histogram::runOnBasicBlock(BasicBlock &BB) {

  const size_t Size = BB.size();

  // If there are too many elements, it would be sensible to write 
  // them directly to disk.
  this->sizes.push_back(Size);

  // The basic block is not changed.
  return false;
}

PreservedAnalyses Histogram::run(llvm::Function &F,
                                 llvm::FunctionAnalysisManager &) {
  bool Changed = runOnFunction(F);

  return (Changed ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all());
}

bool LegacyHistogram::runOnFunction(llvm::Function &F) {
  return Impl.runOnFunction(F);
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getHistogramPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "histogram", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "histogram") {
                    FPM.addPass(Histogram());
                    return true;
                  }
                  return false;
                });

              PB.registerVectorizerStartEPCallback(
                [](llvm::FunctionPassManager &FPM,
                   llvm::PassBuilder::OptimizationLevel O) {
                  FPM.addPass(Histogram());
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getHistogramPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
char LegacyHistogram::ID = 0;

static RegisterPass<LegacyHistogram> X(/*PassArg=*/"legacy-histogram",
                                       /*Name=*/"Histogram",
                                       /*CFGOnly=*/true,
                                       /*is_analysis=*/false);

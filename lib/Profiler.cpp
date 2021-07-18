/*
  Task:
    b) Build a simple profiler that will print the basic blocks which are
        executed more than N times.  To accomplish this, you should write
        an LLVM pass that inserts a call to a function called bb_exec
        inside each basic block, which should take as argument(s) a unique
        identifier of that basic block.  The program should then be linked
        with an actual implementation of the function bb_exec which will
        keep track of how many times each basic block was executed, and will
        print all basic blocks that are executed more than N times. The
        default value for N should be 1000.
*/

#include "Profiler.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/ConstantMerge.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

#define PROFILER_TRAMPOLINE_NAME "profiler_trampoline"
#define PROFILER_BB_EXEC_NAME "bb_exec"

//-----------------------------------------------------------------------------
// Profiler Implementation
//-----------------------------------------------------------------------------

static FunctionType *getFunctionType(LLVMContext &Context) {
  SmallVector<Type *, 3> ArgsType;
  ArgsType.push_back(Type::getInt8PtrTy(Context));
  ArgsType.push_back(Type::getInt8PtrTy(Context));
  ArgsType.push_back(Type::getInt8PtrTy(Context));

  return FunctionType::get(Type::getVoidTy(Context), ArgsType, false);
}

static Function *createBbExec(Module *Module, FunctionType *FT) {

  if (Function *function = Module->getFunction(PROFILER_BB_EXEC_NAME)) {
    return function;
  }

  /*
    ExternalWeakLinkage is the only LinkageType that is working with LD_PRELOAD.
    LLVM expects that such function  not to have a body because it is external.
  */

  Function *F =
      Function::Create(FT, Function::LinkageTypes::ExternalWeakLinkage,
                       PROFILER_BB_EXEC_NAME, *Module);
  F->addFnAttr(llvm::Attribute::AttrKind::OptimizeNone);
  F->addFnAttr(llvm::Attribute::AttrKind::NoInline);
  return F;
}

static Function *createTrampolineFunction(Module *Module) {

  /*
    The trampoline function looks like this:

    void trampoline(i8*, i8*, i8*){
      if (bb_exec){
        bb_exec(i8*, i8*, i8*);
      }
    }

    In this way, the profiled binary can be usable even if there is no bb_exec
    linked. It is intended to be compatible with LD_PRELOAD.
  */

  if (Function *function = Module->getFunction(PROFILER_TRAMPOLINE_NAME)) {
    return function;
  }

  FunctionType *FT = getFunctionType(Module->getContext());
  Function *BbExec = createBbExec(Module, FT);
  LLVMContext &Context = Module->getContext();
  Function *Trampoline =
      Function::Create(FT, Function::LinkageTypes::PrivateLinkage,
                       PROFILER_TRAMPOLINE_NAME, *Module);
  BasicBlock *Entry = BasicBlock::Create(Context, "entry", Trampoline, nullptr);
  BasicBlock *BbNotNull =
      BasicBlock::Create(Context, "not null", Trampoline, nullptr);
  BasicBlock *Exit = BasicBlock::Create(Context, "exit", Trampoline, nullptr);

  IRBuilder<> Builder(Entry);
  Value *cmp = Builder.CreateICmpNE(
      BbExec, ConstantPointerNull::get(FT->getPointerTo()));
  Builder.CreateCondBr(cmp, BbNotNull, Exit);

  // If bb_exec's symbol is defined, then perform the call.
  Builder.SetInsertPoint(BbNotNull);
  SmallVector<Value *, 3> Args;
  Args.push_back(Trampoline->getArg(0));
  Args.push_back(Trampoline->getArg(1));
  Args.push_back(Trampoline->getArg(2));
  Builder.CreateCall(FT, BbExec, Args);
  Builder.CreateBr(Exit);

  Builder.SetInsertPoint(Exit);
  Builder.CreateRetVoid();

  Trampoline->addFnAttr(llvm::Attribute::AttrKind::OptimizeNone);
  Trampoline->addFnAttr(llvm::Attribute::AttrKind::NoInline);

  return Trampoline;
}

bool Profiler::runOnFunction(Function &F) {
  // Do not profile the profiler.
  if (F.getName().equals(PROFILER_BB_EXEC_NAME) ||
      F.getName().equals(PROFILER_TRAMPOLINE_NAME)) {
    return false;
  }

  llvm::errs() << "[Profiler Pass] Transforming function: " << F.getName() << "\n";

  this->trampoline = createTrampolineFunction(F.getParent());

  bool Changed = false;
  for (auto &BB : F) {
    Changed |= runOnBasicBlock(BB);
  }

  return Changed;
}

static std::string getSimpleNodeLabel(const BasicBlock *Node) {
  if (!Node->getName().empty())
    return Node->getName().str();

  std::string Str;
  raw_string_ostream OS(Str);

  Node->printAsOperand(OS, false);
  return OS.str();
}

bool Profiler::runOnBasicBlock(BasicBlock &BB) {

  // TODO: Currently, global constants are duplicated and not re-used.
  //       For instance, the Module's name should be inserted only once.

  IRBuilder<> Builder(&*BB.getFirstInsertionPt());

  const Function *ParentFunction = BB.getParent();
  Constant *FuncName = Builder.CreateGlobalStringPtr(ParentFunction->getName());

  const Module *Module = ParentFunction->getParent();
  Constant *ModuleName = Builder.CreateGlobalStringPtr(Module->getName());

  Constant *BBName = Builder.CreateGlobalStringPtr(getSimpleNodeLabel(&BB));

  SmallVector<Value *, 3> Args;
  Args.push_back(BBName);
  Args.push_back(FuncName);
  Args.push_back(ModuleName);

  Builder.CreateCall(this->trampoline->getFunctionType(), this->trampoline,
                     Args);

  return true;
}

PreservedAnalyses Profiler::run(llvm::Function &F,
                                llvm::FunctionAnalysisManager &) {
  bool Changed = runOnFunction(F);

  return (Changed ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all());
}

bool LegacyProfiler::runOnFunction(llvm::Function &F) {
  return Impl.runOnFunction(F);
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getProfilerPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "profiler", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "profiler") {
                    FPM.addPass(Profiler());
                    return true;
                  }
                  return false;
                });

            PB.registerVectorizerStartEPCallback(
                [](llvm::FunctionPassManager &FPM,
                   llvm::PassBuilder::OptimizationLevel O) {
                  FPM.addPass(Profiler());
                });
          }

  };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getProfilerPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
char LegacyProfiler::ID = 0;

static RegisterPass<LegacyProfiler> X(/*PassArg=*/"legacy-profiler",
                                      /*Name=*/"Profiler",
                                      /*CFGOnly=*/true,
                                      /*is_analysis=*/false);
#pragma once
#ifndef __SYSU_OPTIMIZER_PLUGIN_HH_
#define __SYSU_OPTIMIZER_PLUGIN_HH_

#include <llvm/ADT/MapVector.h>
#include <llvm/IR/AbstractCallSite.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/raw_ostream.h>
// add
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/IteratedDominanceFrontier.h>
#include <stack>
// #include <llvm/IR/IRBuilder.h>
// #include <llvm/Transforms/Utils/PromoteMemToReg.h>

namespace sysu{
    class StaticCallCounter: public llvm::AnalysisInfoMixin<StaticCallCounter>
    {
    private:
        // A special type used by analysis passes to provide an address that
        // identifies that particular analysis pass type.  
        static llvm::AnalysisKey Key;
        friend struct llvm::AnalysisInfoMixin<StaticCallCounter>;
    
    public:
        using Result = llvm::MapVector<const llvm::Function *, unsigned>;
        Result run(llvm::Module &M, llvm::ModuleAnalysisManager &);
    };

    class StaticCallCounterPrinter: public llvm::PassInfoMixin<StaticCallCounterPrinter>
    {
    private:
        llvm::raw_ostream &OS;
    
    public:
        explicit StaticCallCounterPrinter(llvm::raw_ostream &OutS): OS(OutS){}
        llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM);
    };
}
// namespace sysu end

extern "C"{
    llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo();
}

// 类名与继承名需相同
class Mem2RegPass: public llvm::PassInfoMixin<Mem2RegPass>
{
public:
    // explicit Mem2RegPass(){}
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);
};

#endif
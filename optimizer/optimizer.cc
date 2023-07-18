#include "optimizer.hh"
#include <llvm/Passes/PassBuilder.h>

// #define DEBUG 1
// void debug_out(std::string s){
//     if(DEBUG) llvm::errs()<<s<<'\n';
// }
// todo
// std::make_pair

llvm::PreservedAnalyses sysu::StaticCallCounterPrinter::run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM){
    auto DirectCalls = MAM.getResult<sysu::StaticCallCounter>(M);

    OS<<"=================================================\n";
    OS<<"sysu-optimizer: static analysis results\n";
    OS<<"=================================================\n";
    const char *str1 = "NAME", *str2 = "#N DIRECT CALLS";
    OS<<llvm::format("%-20s %-10s\n", str1, str2);
    OS<<"-------------------------------------------------\n";

    for (auto &CallCount: DirectCalls)
    {
        OS<<llvm::format("%-20s %-10lu\n", CallCount.first->getName().str().c_str(), CallCount.second);
    }
    
    OS<<"-------------------------------------------------\n\n";
    return llvm::PreservedAnalyses::all();
}

sysu::StaticCallCounter::Result sysu::StaticCallCounter::run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM){
    llvm::MapVector<const llvm::Function *, unsigned> Res;

    for (auto &Func: M)
    {
        for (auto &BB: Func)
        {
            for (auto &Ins: BB)
            {
                // If this is a call instruction then CB will be not null.
                auto *CB = llvm::dyn_cast<llvm::CallBase>(&Ins);
                if (nullptr == CB)
                {
                    continue;
                }
                
                // If CB is a direct function call then DirectInvoc will be not null.
                auto DirectInvoc = CB->getCalledFunction();
                if (nullptr == DirectInvoc)
                {
                    continue;
                }
                
                // We have a direct function call - update the count for the function
                // being called.
                auto CallCount = Res.find(DirectInvoc);
                if (Res.end() == CallCount)
                {
                    CallCount = Res.insert({DirectInvoc, 0}).first;
                }
                ++CallCount->second;
            }   
        }
    }
    
    return Res;
}

llvm::AnalysisKey sysu::StaticCallCounter::Key;

extern "C"{
    llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo(){
        return {
            LLVM_PLUGIN_API_VERSION, "sysu-optimizer-pass", LLVM_VERSION_STRING, [](llvm::PassBuilder &PB){
                // #1 REGISTRATION FOR "opt -passes=sysu-optimizer-pass"
                PB.registerPipelineParsingCallback(
                    [&](llvm::StringRef Name, llvm::ModulePassManager &MPM,
                        llvm::ArrayRef<llvm::PassBuilder::PipelineElement>){
                        if (Name == "sysu-optimizer-pass")
                        {
                            MPM.addPass(sysu::StaticCallCounterPrinter(llvm::errs()));
                            return true;
                        }   
                        return false;
                    }
                );

                // #2 REGISTRATION FOR
                // "MAM.getResult<sysu::StaticCallCounter>(Module)"
                PB.registerAnalysisRegistrationCallback(
                    [](llvm::ModuleAnalysisManager &MAM){
                        MAM.registerPass([&] { return sysu::StaticCallCounter();});
                    }
                );
            }
        };
    }
}

/********************Mem2Reg*********************/
// phi节点对应的alloca
llvm::SmallDenseMap<llvm::PHINode *, llvm::AllocaInst *> phi2alloca;
// alloca变量对应的值栈
llvm::SmallDenseMap<llvm::AllocaInst *, std::stack<llvm::Value *>> value_stack_of_alloca; 

bool is_promotable(llvm::AllocaInst *alloca_inst){
    // todo: 加入更多的情况
    if (alloca_inst->getAllocatedType()->isArrayTy())
    {
        // 不优化数组
        return false;
    }
    
    for (auto user_it = alloca_inst->user_begin(); user_it != alloca_inst->user_end(); ++user_it)
    {
        // *user_it -> type: llvm::User*
        if (llvm::LoadInst *load_inst = llvm::dyn_cast<llvm::LoadInst>(*user_it))
        {
            // load
            if (alloca_inst->getAllocatedType() != load_inst->getType())
            {
                // 申请类型与取数类型不一致
                return false;
            }
        }
        
        if (llvm::StoreInst *store_inst = llvm::dyn_cast<llvm::StoreInst>(*user_it))
        {
            // store
            if (alloca_inst->getAllocatedType() != store_inst->getValueOperand()->getType())
            {
                // 类型不一致
                return false;
            }
        }

        // if (llvm::BitCastInst *bitcast_inst = llvm::dyn_cast<llvm::BitCastInst>(*user_it))
        // {
        //     // bitcase
        //     return false;
        // }
        
        // todo
    }
    
    return true;
}

void insert_phi_node(llvm::AllocaInst *alloca_inst, const llvm::SmallVectorImpl<llvm::BasicBlock *> &phi_node_blocks){
    // debug_out("insert_phi_node");
    unsigned int v = 0;
    llvm::Type *type = alloca_inst->getAllocatedType();
    std::string name = alloca_inst->getName().str();

    for (llvm::BasicBlock *block: phi_node_blocks)
    {
        // 遍历block
        // 创建phi_node
        llvm::PHINode *phi_node = llvm::PHINode::Create(type, 0, name + std::to_string(v));
        ++v;
        phi2alloca.insert(std::make_pair(phi_node, alloca_inst));
        block->getInstList().push_front(phi_node); // phi_node 插在最前面
        // // 找到phi_node的插入点：load、store之后，即block中alloca user之后
        // for (auto inst_it = block->getInstList().rbegin(); inst_it != block->getInstList().rend(); ++inst_it)
        // {
        //     // 遍历inst
        //     for (auto user_it = alloca_inst->user_begin(); user_it != alloca_inst->user_end(); ++user_it)
        //     {
        //         if (&(*inst_it) == llvm::dyn_cast<llvm::Instruction>(*user_it))
        //         {
        //             // 创建phi_node
        //             llvm::PHINode *phi_node = llvm::PHINode::Create(type, 0, name + std::to_string(v));
        //             ++v;
        //             phi2alloca.insert(std::make_pair(phi_node, alloca_inst));
        //             block->getInstList().insertAfter(inst_it, phi_node); // 类型不匹配，而且block->getInstList()循环中会发生变化，需要特殊处理                
        //         }
        //     }
        // }
    }
}

void myrename(llvm::DomTreeNode *dom_tree_node){
    // debug_out("myrename");
    // 本质是递归函数

    // 1. 处理当前块的load、store、phi_node
    llvm::BasicBlock *cur_block = dom_tree_node->getBlock();
    // 待删除的load, 不要在处理cur_block时删除load，会导致迭代器发生变化
    llvm::SmallVector<llvm::LoadInst *, 8> to_remove_loads;
    for (llvm::Instruction &inst: *cur_block)
    {
        if (llvm::LoadInst *load_inst = llvm::dyn_cast<llvm::LoadInst>(&inst))
        {
            // load
            if (llvm::AllocaInst *alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(load_inst->getPointerOperand()))
            {
                // load的变量地址是alloca申请的
                if (value_stack_of_alloca.find(alloca_inst) != value_stack_of_alloca.end())
                {
                    // 值栈中包括该变量地址
                    std::stack<llvm::Value *> value_stack = value_stack_of_alloca.find(alloca_inst)->getSecond();
                    // 用栈的顶部值替换load变量出现的位置
                    load_inst->replaceAllUsesWith(value_stack.top());
                    to_remove_loads.emplace_back(load_inst);
                }
            }
        }
        else if (llvm::StoreInst *store_inst = llvm::dyn_cast<llvm::StoreInst>(&inst))
        {
            // store
            if (llvm::AllocaInst *alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(store_inst->getPointerOperand()))
            {
                // store的变量地址是alloca申请的
                if (value_stack_of_alloca.find(alloca_inst) != value_stack_of_alloca.end())
                {
                    // 值栈中包括该变量地址
                    // 将store的值存入值栈顶部
                    llvm::Value *new_value = store_inst->getValueOperand();
                    value_stack_of_alloca.find(alloca_inst)->second.push(new_value);
                }
            }
        }
        else if (llvm::PHINode *phi_node = llvm::dyn_cast<llvm::PHINode>(&inst))
        {
            // phi
            if (phi2alloca.find(phi_node) != phi2alloca.end())
            {
                // phi节点存在alloca
                llvm::AllocaInst *alloca_inst = phi2alloca.find(phi_node)->getSecond();
                if (value_stack_of_alloca.find(alloca_inst) != value_stack_of_alloca.end())
                {
                    // 值栈中包括该变量地址
                    // 将phi的值存入值栈顶部
                    value_stack_of_alloca.find(alloca_inst)->second.push(phi_node);
                }
            }
        }
    }
    // 删除待删除的load
    // if(to_remove_loads.empty()) debug_out("no loads remove");
    for (llvm::LoadInst *load_inst: to_remove_loads)
    {
        load_inst->eraseFromParent();
    }
    
    // 2. 处理后继块
    for (llvm::BasicBlock *suc_block: llvm::successors(cur_block))
    {
        for (llvm::Instruction &inst: *suc_block)
        {
            if (llvm::PHINode *phi_node = llvm::dyn_cast<llvm::PHINode>(&inst))
            {
                // inst为phi
                // 找到phi对应的alloca，之后找到alloca对应的值栈，将栈顶值加入phi节点
                if (phi2alloca.find(phi_node) != phi2alloca.end())
                {
                    llvm::AllocaInst *alloca_inst = phi2alloca.find(phi_node)->getSecond();
                    if (value_stack_of_alloca.find(alloca_inst) != value_stack_of_alloca.end())
                    {
                        llvm::Value *phi_value = value_stack_of_alloca.find(alloca_inst)->getSecond().top();
                        phi_node->addIncoming(phi_value, cur_block);
                    }
                }
            }
        }
    }
    
    // 3. 递归子节点
    for (llvm::DomTreeNode *child_node: dom_tree_node->children())
    {
        // debug
        if (child_node != nullptr)
        {
            myrename(child_node);
        }
    }
    
    // 4. 维护值栈，跳出当前块之后就需要将值弹出，包括store和phi
    // 待删除的store
    llvm::SmallVector<llvm::StoreInst *, 8> to_remove_stores;
    for (llvm::Instruction &inst: *cur_block)
    {
        if (llvm::StoreInst *store_inst = llvm::dyn_cast<llvm::StoreInst>(&inst))
        {
            // store
            if (llvm::AllocaInst *alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(store_inst->getPointerOperand()))
            {
                // store的变量地址是alloca申请的
                if (value_stack_of_alloca.find(alloca_inst) != value_stack_of_alloca.end())
                {
                    // 值栈中包括该变量地址
                    // 将store的值弹出
                    if (!value_stack_of_alloca.find(alloca_inst)->second.empty())
                    {
                        // 先检查栈值是否为空
                        value_stack_of_alloca.find(alloca_inst)->second.pop();
                    }
                }
                to_remove_stores.emplace_back(store_inst);
            }
        }
        else if (llvm::PHINode *phi_node = llvm::dyn_cast<llvm::PHINode>(&inst))
        {
            // phi
            if (phi2alloca.find(phi_node) != phi2alloca.end())
            {
                // phi的变量地址是alloca申请的
                llvm::AllocaInst *alloca_inst = phi2alloca.find(phi_node)->getSecond();
                if (value_stack_of_alloca.find(alloca_inst) != value_stack_of_alloca.end())
                {
                    // 值栈中包括该变量地址
                    // 将phi的值弹出
                    if (!value_stack_of_alloca.find(alloca_inst)->second.empty())
                    {
                        value_stack_of_alloca.find(alloca_inst)->second.pop();
                    }
                }
            }
        }
    }
    // 删除待删除的store
    for (llvm::StoreInst *store_inst: to_remove_stores)
    {
        store_inst->eraseFromParent();
    }
}

void promote(llvm::DominatorTree &DT, const llvm::SmallVectorImpl<llvm::AllocaInst *> &alloca_list){
    // debug_out("promote");
    // 插入phi_node
    for (llvm::AllocaInst *alloca_inst: alloca_list)
    {
        // 找到使用alloca指令申请的变量的BLOCK
        llvm::SmallPtrSet<llvm::BasicBlock *, 8> defining_blocks;
        for (auto user_it = alloca_inst->user_begin(); user_it != alloca_inst->user_end(); ++user_it)
        {
            if (llvm::StoreInst *store_inst = llvm::dyn_cast<llvm::StoreInst>(*user_it))
            {
                defining_blocks.insert(store_inst->getParent());
            }
        }

        // ForwardIDF，用于计算blocks的支配关系
        llvm::ForwardIDFCalculator ForwardIDF(DT);
        ForwardIDF.setDefiningBlocks(defining_blocks);

        // 将计算结果存储到PHINodeBlocks
        llvm::SmallVector<llvm::BasicBlock *, 8> phi_node_blocks;
        ForwardIDF.calculate(phi_node_blocks);
        
        // 将PHInode插入PHINodeBlocks
        insert_phi_node(alloca_inst, phi_node_blocks);
    }

    // 对于一些块，是CFG可能会经过的，但是也可能不会经过，所以需要在值栈中存入undefined值，防止访问不到值而出错
    for (llvm::AllocaInst *alloca_inst: alloca_list)
    {
        std::stack<llvm::Value *> value_stack;
        value_stack.push(llvm::UndefValue::get(alloca_inst->getAllocatedType()));
        value_stack_of_alloca.insert(std::make_pair(alloca_inst, value_stack));
    }
    
    // 变量重命名
    myrename(DT.getRootNode());

    // 删除alloca指令
    for (llvm::AllocaInst *alloca_inst: alloca_list)
    {
        alloca_inst->eraseFromParent();
    }
}

llvm::PreservedAnalyses Mem2RegPass::run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM){
    bool is_promoted = false;
    // 支配树
    llvm::DominatorTree &DT = FAM.getResult<llvm::DominatorTreeAnalysis>(F);

    llvm::SmallVector<llvm::AllocaInst *, 8> promotable_alloca_list;
    while (1)
    {
        // 每次迭代之前都需要清空
        promotable_alloca_list.clear();
        llvm::BasicBlock &entry = F.getEntryBlock(); // generator阶段保证了alloca只在EntryBlock
        for (llvm::Instruction &inst: entry)
        {
            if (llvm::AllocaInst *alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(&inst))
            {
                if (is_promotable(alloca_inst))
                // if (llvm::isAllocaPromotable(alloca_inst))
                {
                    promotable_alloca_list.emplace_back(alloca_inst);
                }               
            }

        }       

        if (promotable_alloca_list.empty())
        {
            // 迭代结束，已无可继续优化的alloca
            break;
        }
        
        promote(DT, promotable_alloca_list);
        is_promoted = true;
    }
    
    if (!is_promoted)
    {
        // 未优化
        // debug_out("is not promoted");
        return llvm::PreservedAnalyses::all();
    }
    
    // debug_out("promoted");
    llvm::PreservedAnalyses newPA;
    newPA.preserveSet<llvm::CFGAnalyses>();
    return newPA;

    // return llvm::PreservedAnalyses::all();
}
/********************Mem2Reg end*********************/
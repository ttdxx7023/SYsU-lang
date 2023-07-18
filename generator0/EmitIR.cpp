#include "EmitIR.hpp"

// --------TranslationUnitDecl--------
void EmitIR::TranslationUnitDeclToIR(TranslationUnitDecl *root){
    // debug_out("TranslationUnitDeclToIR");
    if(root == nullptr) return;

    // 全局变量定义
    is_global_var_decl = true;
    for (auto it: root->global_var_decls)
    {
        GlobalVarDeclToIR(it);
    }
    is_global_var_decl = false;

    // 函数
    for (auto it: root->function_decls)
    {
        FunctionDeclToIR(it);
    }
}

// --------基类--------
void EmitIR::DeclToIR(Decl *ptr){
    // debug_out("DeclToIR");
    // 局部变量
    if(auto var_decl = dynamic_cast<VarDecl*>(ptr)) VarDeclToIR(var_decl);
    // 函数参数
    else if (auto parm_var_decl = dynamic_cast<ParmVarDecl*>(ptr)) ParmVarDeclToIR(parm_var_decl);
    // 函数
    else if (auto function_decl = dynamic_cast<FunctionDecl*>(ptr)) FunctionDeclToIR(function_decl);
}

llvm::Value* EmitIR::ExprToIR(Expr *ptr, llvm::Value *addr){
    // debug_out("ExprToIR");
    if(auto expr = dynamic_cast<IntegerLiteral*>(ptr))
        return IntegerLiteralToIR(expr);
    else if(auto expr = dynamic_cast<FloatingLiteral*>(ptr))
        return FloatingLiteralToIR(expr);
    else if(auto expr = dynamic_cast<CharacterLiteral*>(ptr))
        return CharacterLiteralToIR(expr);
    else if(auto expr = dynamic_cast<StringLiteral*>(ptr))
        return StringLiteralToIR(expr);
    else if(auto expr = dynamic_cast<UnaryExpr*>(ptr))
        return UnaryExprToIR(expr);
    else if(auto expr = dynamic_cast<BinaryExpr*>(ptr))
        return BinaryExprToIR(expr);
    else if(auto expr = dynamic_cast<ParenExpr*>(ptr))
        return ParenExprToIR(expr);
    else if(auto expr = dynamic_cast<DeclRefExpr*>(ptr))
        return DeclRefExprToIR(expr);
    else if(auto expr = dynamic_cast<InitListExpr*>(ptr))
        return InitListExprToIR(expr, addr);
    else if(auto expr = dynamic_cast<ArraySubscriptExpr*>(ptr))
        return ArraySubscriptExprToIR(expr);
    else if(auto expr = dynamic_cast<ImplicitCastExpr*>(ptr))
        return ImplicitCastExprToIR(expr);
    else if(auto expr = dynamic_cast<CallExpr*>(ptr))
        return CallExprToIR(expr);

    return nullptr;    
}

void EmitIR::StmtToIR(Stmt *ptr, llvm::BasicBlock *continue_block, llvm::BasicBlock *break_block){
    // debug_out("StmtToIR");
    // if(break_block!=nullptr) debug_out(break_block->getName().str());
    if(auto stmt = dynamic_cast<CompoundStmt*>(ptr)) CompoundStmtToIR(stmt, continue_block, break_block);
    else if(auto stmt = dynamic_cast<NullStmt*>(ptr)) NullStmtToIR(stmt);
    else if(auto stmt = dynamic_cast<ReturnStmt*>(ptr)) ReturnStmtToIR(stmt);
    else if(auto stmt = dynamic_cast<DeclStmt*>(ptr)) DeclStmtToIR(stmt);
    else if(auto stmt = dynamic_cast<ExprStmt*>(ptr)) ExprStmtToIR(stmt);
    else if(auto stmt = dynamic_cast<IfStmt*>(ptr)) IfStmtToIR(stmt, continue_block, break_block);
    else if(auto stmt = dynamic_cast<WhileStmt*>(ptr)) WhileStmtToIR(stmt);
    else if(auto stmt = dynamic_cast<DoStmt*>(ptr)) DoStmtToIR(stmt);
    else if(auto stmt = dynamic_cast<BreakStmt*>(ptr)) BreakStmtToIR(stmt, break_block);
    else if(auto stmt = dynamic_cast<ContinueStmt*>(ptr)) ContinueStmtToIR(stmt, continue_block);
}

// --------Decl--------
void EmitIR::GlobalVarDeclToIR(VarDecl* ptr){
    // debug_out("GlobalVarDeclToIR");
    // 插入全局变量
    module.getOrInsertGlobal(ptr->name, ptr->type.type);
    auto var = module.getGlobalVariable(ptr->name);
    var->setConstant(ptr->type.is_const);
    if (ptr->init_expr != nullptr)
    {
        // 初始化
        auto init_value = ExprToIR(ptr->init_expr);
        // constant
        if(auto constant = llvm::cast<llvm::Constant>(init_value)) var->setInitializer(constant);
        // array
        else if(auto array = llvm::cast<llvm::ConstantArray>(init_value)) var->setInitializer(array);
    }
    else{
        // 未初始化: 默认初始化
        var->setInitializer(llvm::Constant::getNullValue(ptr->type.type));
    }
}

void EmitIR::registerPtr(Obj *ptr, llvm::AllocaInst *alloca_inst){
    symbol_table.emplace(ptr, alloca_inst);
}

void EmitIR::VarDeclToIR(VarDecl *ptr){
    // debug_out("VarDeclToIR");
    // 把所有变量的地址统一在entry_block最前面声明申请
    // 获取正在插入的块
    auto cur_block = ir_builder.GetInsertBlock();
    // 获取插入块的所属函数
    auto function = cur_block->getParent();
    // 获取函数的entry_block
    // auto entry_block = (*function).getEntryBlock();
    auto entry_block = &function->getEntryBlock();
    ir_builder.SetInsertPoint(entry_block);
    // 找到entry_block的最后一个alloca指令，在这之后插入新的alloca指令
    auto new_alloca_inst = ir_builder.CreateAlloca(ptr->type.type, nullptr, ptr->name);
    for (auto& it: *entry_block)
    {
        if(it.getOpcode() != llvm::Instruction::Alloca){
            new_alloca_inst->moveBefore(&it);
            break;
        }
    }
    // 注册入符号表
    registerPtr(ptr, new_alloca_inst);
    // 在当前块处理初始化
    ir_builder.SetInsertPoint(cur_block);
    if (ptr->init_expr != nullptr)
    {
        if(ptr->type.type->isArrayTy()){
            // 数组和字符串都会是array_type
            if(auto init_list_expr = dynamic_cast<InitListExpr*>(ptr->init_expr)){
                // 数组初始化
                // 数组初始化需要先memset!先计算数组大小和内存大小
                int array_size = ptr->type.type->getArrayNumElements();
                auto element_type = ptr->type.type->getArrayElementType();
                while (element_type->isArrayTy())
                {
                    array_size = array_size*element_type->getArrayNumElements();
                    element_type = element_type->getArrayElementType();
                }
                int mem_size = array_size*element_type->getPrimitiveSizeInBits();
                // 注意参数
                ir_builder.CreateMemSet(new_alloca_inst, ir_builder.getInt8(0), mem_size>>3, llvm::MaybeAlign(16));
                InitListExprToIR(init_list_expr, new_alloca_inst);
            }
            else if(auto string_literal = dynamic_cast<StringLiteral*>(ptr->init_expr)){
                // 字符串初始化
                // 字符串初始化使用memcopy
                auto init_string = StringLiteralToIR(string_literal);
                // 字符数量
                auto size = ptr->type.type->getArrayNumElements();
                // Align: 对齐方式
                ir_builder.CreateMemCpy(new_alloca_inst, llvm::MaybeAlign(1), init_string, llvm::MaybeAlign(1), size);
            }
        }
        else{
            // 表达式或其他字面量初始化
            auto init_value = ExprToIR(ptr->init_expr);
            ir_builder.CreateStore(init_value, new_alloca_inst);
        }
    }
}

void EmitIR::ParmVarDeclToIR(ParmVarDecl *ptr){
    // debug_out("ParmVarDeclToIR");
    // 把所有变量的地址统一在entry_block最前面声明申请
    // 获取正在插入的块
    auto cur_block = ir_builder.GetInsertBlock();
    // 获取插入块的所属函数
    auto function = cur_block->getParent();
    // 获取函数的entry_block
    // auto entry_block = (*function).getEntryBlock();
    auto entry_block = &function->getEntryBlock();
    ir_builder.SetInsertPoint(entry_block);
    // 找到entry_block的最后一个alloca指令，在这之后插入新的alloca指令
    auto new_alloca_inst = ir_builder.CreateAlloca(ptr->type.type, nullptr, ptr->name);
    for (auto& it: *entry_block)
    {
        if(it.getOpcode() != llvm::Instruction::Alloca){
            new_alloca_inst->moveBefore(&it);
            break;
        }
    }    
    // 注册入符号表
    registerPtr(ptr, new_alloca_inst);
}

void EmitIR::FunctionDeclToIR(FunctionDecl *ptr){
    // debug_out("FunctionDeclToIR");
    auto function = module.getFunction(ptr->name);
    if (function == nullptr)
    {
        // 函数未声明
        function = llvm::Function::Create(ptr->function_type, llvm::Function::ExternalLinkage, ptr->name, &module);
        // 设置参数的名字 undo
        int arg_idx = 0;
        for (auto& arg: function->args())
        {
            arg.setName(ptr->parm_var_decls[arg_idx]->name);
            arg_idx++;
        }
        module.getOrInsertFunction(ptr->name, ptr->function_type);
    }
    // debug_out(function->getName().str());
    if (ptr->block != nullptr)
    {
        // 创建entry_block
        auto entry_block = llvm::BasicBlock::Create(context, "entry", function);
        ir_builder.SetInsertPoint(entry_block);

        // 函数参数
        std::vector<llvm::AllocaInst*> alloca_insts;
        // alloca
        for (auto parm: ptr->parm_var_decls)
        {
            auto new_alloca_inst = ir_builder.CreateAlloca(parm->type.type, nullptr, parm->name);
            registerPtr(parm, new_alloca_inst);
            alloca_insts.emplace_back(new_alloca_inst);
        }
        // store
        int arg_idx = 0;
        for (auto& arg: function->args())
        {
            ir_builder.CreateStore(&arg, alloca_insts[arg_idx]);
            arg_idx++;
        }
        
        // 函数体
        StmtToIR(ptr->block);
        
        // 当最后一个基本块为空或不含返回语句时，需要手动添加一个返回语句
        // auto last_block = function->getBasicBlockList().back();
        auto& last_block = function->getBasicBlockList().back();
        if (last_block.empty() || !llvm::isa<llvm::ReturnInst>(last_block.back()))
        {
            auto return_type = ptr->function_type->getReturnType();
            if (return_type->isVoidTy()) ir_builder.CreateRetVoid();
            else ir_builder.CreateRet(llvm::Constant::getNullValue(return_type));
        }
    }
    llvm::verifyFunction(*function);
}

// --------Expr--------
llvm::Value* EmitIR::IntegerLiteralToIR(IntegerLiteral *ptr){
    // debug_out("IntegerLiteralToIR");
    return llvm::ConstantInt::get(ptr->type.type, ptr->value);
}

llvm::Value* EmitIR::FloatingLiteralToIR(FloatingLiteral *ptr){
    // debug_out("FloatingLiteralToIR");
    return llvm::ConstantFP::get(ptr->type.type, ptr->value);
}

llvm::Value* EmitIR::CharacterLiteralToIR(CharacterLiteral *ptr){
    // debug_out("CharacterLiteralToIR");
    return llvm::ConstantInt::get(ptr->type.type, ptr->value);
}

llvm::Value* EmitIR::StringLiteralToIR(StringLiteral *ptr){
    // debug_out("StringLiteralToIR");
    int real_size = std::min(ptr->value.size(), ptr->type.type->getArrayNumElements()-1);
    std::string real_str(real_size, ' ');
    for (int i = 0; i < real_size; i++)
    {
        real_str[i] = ptr->value[i];
    }
    auto value = llvm::ConstantDataArray::getString(context, real_str);
    return new llvm::GlobalVariable(module, value->getType(), true, llvm::GlobalValue::PrivateLinkage, value);
}

llvm::Value* EmitIR::ValueToBool(llvm::Value* value){
    auto size = value->getType()->getPrimitiveSizeInBits();
    if(size == 1) return value;

    if (value->getType()->isIntegerTy())
    {
        // 整型与0比较
        auto zero = llvm::ConstantInt::get(context, llvm::APInt(size, 0));
        value = ir_builder.CreateICmpNE(value, zero);   
    }
    else if (value->getType()->isFloatingPointTy())
    {
        // 浮点型与0.0比较
        auto zero = llvm::ConstantFP::get(context, llvm::APFloat(0.0));
        value = ir_builder.CreateFCmpONE(value, zero);
    }
    
    return value;
}

llvm::Value* EmitIR::UnaryExprToIR(UnaryExpr *ptr){
    // debug_out("UnaryExprToIR");
    auto expr_value = ExprToIR(ptr->expr);
    if (ptr->opcode == "+")
    {
        return expr_value;
    }
    else if (ptr->opcode == "-")
    {
        // 小于32位做0扩展
        if (expr_value->getType()->getPrimitiveSizeInBits() < 32)
        {
            expr_value = ir_builder.CreateZExt(expr_value, ir_builder.getInt32Ty(), expr_value->getName());
        }
        if (expr_value->getType()->isIntegerTy())
        {
            // 整型
            return ir_builder.CreateNeg(expr_value);
        }
        else if (expr_value->getType()->isFloatingPointTy())
        {
            // 浮点
            return ir_builder.CreateFNeg(expr_value);
        }
    }
    else if (ptr->opcode == "!")
    {
        return ir_builder.CreateNot(ValueToBool(expr_value));
    }

    return nullptr;
}

llvm::Value* EmitIR::ANDExprToIR(BinaryExpr *ptr){
    auto lhs = ExprToIR(ptr->lhs);
    auto and_lhs = ir_builder.GetInsertBlock();
    auto function = and_lhs->getParent();
    auto and_rhs = llvm::BasicBlock::Create(context, "and.rhs", function);
    auto and_end = llvm::BasicBlock::Create(context, "and.end");
    // and_lhs
    // auto lhs = ExprToIR(ptr->lhs);
    auto lhs_bool_value = ValueToBool(lhs);
    // and短路处理: 如果lhs为false，无需进入and_rhs，计算rhs的值
    ir_builder.CreateCondBr(lhs_bool_value, and_rhs, and_end);
    // and_rhs
    ir_builder.SetInsertPoint(and_rhs);
    // 一定要先设置insertPoint, 再计算rhs
    auto rhs = ExprToIR(ptr->rhs);
    auto rhs_bool_value = ValueToBool(rhs);
    ir_builder.CreateBr(and_end);
    // and_end
    // /workspace/SYsU-lang/tester/function_test2020/38_if_complex_expr.sysu.c
    // 插入点可能会发生变化！
    auto cur_block = ir_builder.GetInsertBlock();
    function->getBasicBlockList().push_back(and_end);
    ir_builder.SetInsertPoint(and_end);
    auto phi_node = ir_builder.CreatePHI(llvm::Type::getInt1Ty(context), 2);
    // All operands to PHI node must be the same type as the PHI node!
    phi_node->addIncoming(ir_builder.getInt1(0), and_lhs);
    phi_node->addIncoming(rhs_bool_value, cur_block);

    return phi_node;
}

llvm::Value* EmitIR::ORExprToIR(BinaryExpr *ptr){
    // 一定要先执行lhs, 才能将rhs放在function的blocklist后面
    // /workspace/SYsU-lang/tester/function_test2020/46_and_prior_or.sysu.c
    // &&优先于||: 优先执行||左/右的&&，才能增加or_rhs
    auto lhs = ExprToIR(ptr->lhs);
    auto or_lhs = ir_builder.GetInsertBlock();
    auto function = or_lhs->getParent();
    auto or_rhs = llvm::BasicBlock::Create(context, "or.rhs", function);
    auto or_end = llvm::BasicBlock::Create(context, "or.end");
    // or_lhs
    // auto lhs = ExprToIR(ptr->lhs);
    auto lhs_bool_value = ValueToBool(lhs);
    // or短路处理: 如果lhs为true，无需进入or_rhs，计算rhs的值
    ir_builder.CreateCondBr(lhs_bool_value, or_end, or_rhs);
    // or_rhs
    ir_builder.SetInsertPoint(or_rhs);
    auto rhs = ExprToIR(ptr->rhs);
    auto rhs_bool_value = ValueToBool(rhs);
    ir_builder.CreateBr(or_end);
    // or_end
    auto cur_block = ir_builder.GetInsertBlock();
    function->getBasicBlockList().push_back(or_end);
    ir_builder.SetInsertPoint(or_end);
    auto phi_node = ir_builder.CreatePHI(llvm::Type::getInt1Ty(context), 2);
    phi_node->addIncoming(ir_builder.getInt1(1), or_lhs);
    phi_node->addIncoming(rhs_bool_value, cur_block);

    return phi_node;
}

llvm::Value* EmitIR::BinaryExprToIR(BinaryExpr *ptr){
    // debug_out("BinaryExprToIR");
    if (ptr->opcode == "&&")
    {
        return ANDExprToIR(ptr);
    }
    else if (ptr->opcode == "||")
    {
        return ORExprToIR(ptr);
    }
    else
    {
        auto lhs = ExprToIR(ptr->lhs);
        auto rhs = ExprToIR(ptr->rhs);
        if(ptr->opcode == "="){
            // 赋值: 注意参数位置
            // debug_out("DeclRef: CreateStore");
            return ir_builder.CreateStore(rhs, lhs);
        }
        else
        {
            bool is_int = lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy();
            auto lsize = lhs->getType()->getPrimitiveSizeInBits();
            auto rsize = rhs->getType()->getPrimitiveSizeInBits();
            // 运算对齐
            if(lsize < rsize) lhs = ir_builder.CreateZExt(lhs, rhs->getType());
            else if (lsize > rsize) rhs = ir_builder.CreateZExt(rhs, lhs->getType());
            
            if(ptr->opcode == "+"){
                if(is_int) return ir_builder.CreateAdd(lhs, rhs);
                return ir_builder.CreateFAdd(lhs, rhs);
            }
            else if(ptr->opcode == "-"){
                if(is_int) return ir_builder.CreateSub(lhs, rhs);
                return ir_builder.CreateFSub(lhs, rhs);
            }
            else if(ptr->opcode == "*"){
                if(is_int) return ir_builder.CreateMul(lhs, rhs);
                return ir_builder.CreateFMul(lhs, rhs);
            }
            else if(ptr->opcode == "/"){
                if(is_int) return ir_builder.CreateSDiv(lhs, rhs);
                return ir_builder.CreateFDiv(lhs, rhs);
            }
            else if(ptr->opcode == "%"){
                if(is_int) return ir_builder.CreateSRem(lhs, rhs);
                return ir_builder.CreateFRem(lhs, rhs);
            }
            else if(ptr->opcode == "=="){
                if(is_int) return ir_builder.CreateICmpEQ(lhs, rhs);
                return ir_builder.CreateFCmpOEQ(lhs, rhs);
            }
            else if(ptr->opcode == "!="){
                if(is_int) return ir_builder.CreateICmpNE(lhs, rhs);
                return ir_builder.CreateFCmpONE(lhs, rhs);
            }
            else if(ptr->opcode == "<"){
                if(is_int) return ir_builder.CreateICmpSLT(lhs, rhs);
                return ir_builder.CreateFCmpOLT(lhs, rhs);
            }
            else if(ptr->opcode == "<="){
                if(is_int) return ir_builder.CreateICmpSLE(lhs, rhs);
                return ir_builder.CreateFCmpOLE(lhs, rhs);
            }
            else if(ptr->opcode == ">"){
                if(is_int) return ir_builder.CreateICmpSGT(lhs, rhs);
                return ir_builder.CreateFCmpOGT(lhs, rhs);
            }
            else if(ptr->opcode == ">="){
                if(is_int) return ir_builder.CreateICmpSGE(lhs, rhs);
                return ir_builder.CreateFCmpOGE(lhs, rhs);
            }
        }
    }
    
    return nullptr;
}

llvm::Value* EmitIR::ParenExprToIR(ParenExpr *ptr){
    // debug_out("ParenExprToIR");
    return ExprToIR(ptr->expr);
}

llvm::Value* EmitIR::DeclRefExprToIR(DeclRefExpr *ptr){
    // debug_out("DeclRefExprToIR");
    if (is_global_var_decl)
    {
        // 使用全局变量给全局变量初始化, 无需创建store指令，直接返回引用的值！
        auto var = module.getGlobalVariable(ptr->ref->name);
        return var->getInitializer();
    }
    
    auto ref_var_ptr_and_addr = symbol_table.find(ptr->ref);
    if (ref_var_ptr_and_addr == symbol_table.end())
    {
        // 使用全局变量初始化
        // debug_out(ptr->ref->name);
        // debug_out("use global");
        auto var = module.getGlobalVariable(ptr->ref->name);
        return var;
    }
    return ref_var_ptr_and_addr->second;
}

llvm::Value* EmitIR::InitListExprToIR(InitListExpr *ptr, llvm::Value *addr){
    // debug_out("InitListExprToIR");
    if (is_global_var_decl)
    {
        // 全局变量使用数组初始化
        std::vector<llvm::Constant*> value_array;
        // 含值
        for (auto it: ptr->sub_init_exprs)
        {
            value_array.emplace_back(llvm::cast<llvm::Constant>(ExprToIR(it)));
        }
        // 补全默认初始值
        auto array_type = llvm::cast<llvm::ArrayType>(ptr->type.type);
        auto total_size = array_type->getArrayNumElements();
        auto element_type = array_type->getArrayElementType();
        for (size_t i = value_array.size(); i < total_size; i++)
        {
            value_array.emplace_back(llvm::Constant::getNullValue(element_type));
        }
        return llvm::ConstantArray::get(array_type, value_array);
    }
    
    for (int i = 0; i < ptr->sub_init_exprs.size(); i++)
    {
        auto new_addr = ir_builder.CreateInBoundsGEP(addr, {ir_builder.getInt64(0), ir_builder.getInt64(i)});
        if(ptr->type.type->getArrayElementType()->isArrayTy()){
            // 多维数组
            if (auto sub_init_list_expr = dynamic_cast<InitListExpr*>(ptr->sub_init_exprs[i]))
            {
                InitListExprToIR(sub_init_list_expr, new_addr);
            }
        }
        else
        {
            auto value = ExprToIR(ptr->sub_init_exprs[i]);
            ir_builder.CreateStore(value, new_addr);
        }
    }

    return nullptr;
}

llvm::Value* EmitIR::ArraySubscriptExprToIR(ArraySubscriptExpr *ptr){
    // debug_out("ArraySubscriptExprToIR");
    auto sub_expr = ExprToIR(ptr->sub_expr);
    std::vector<llvm::Value*> indexs;
    // 判断数组是做左值还是右值
    auto element_type = sub_expr->getType()->getPointerElementType();
    if (element_type->isPointerTy())
    {
        // 右值，读取数组，无需增加基地址
        sub_expr = ir_builder.CreateLoad(sub_expr);
    }
    else if (element_type->isArrayTy())
    {
        // 左值, 设置取值基地址为0
        indexs.emplace_back(ir_builder.getInt64(0));
    }
    // 处理index
    for (auto it: ptr->indexs)
    {
        auto index = ExprToIR(it);
        index = ir_builder.CreateSExt(index, llvm::Type::getInt64Ty(context));
        indexs.emplace_back(index);
    }
    
    return ir_builder.CreateInBoundsGEP(sub_expr, indexs);
}

llvm::Value* EmitIR::ImplicitCastExprToIR(ImplicitCastExpr *ptr){
    // debug_out("ImplicitCastExprToIR");
    std::string cast_kind = ptr->cast_kind;
    auto expr_value = ExprToIR(ptr->sub_expr);
    auto to_type = ptr->type.type;

    if(cast_kind == "IntegralToFloating"){
        expr_value = ir_builder.CreateSIToFP(expr_value, to_type);
    }
    else if(cast_kind == "FloatingToIntegral"){
        expr_value = ir_builder.CreateFPToSI(expr_value, to_type);
    }
    else if(cast_kind == "IntegralCast"){
        // int -> unsigned int
        expr_value = ir_builder.CreateIntCast(expr_value, to_type, true);
    }
    else if(cast_kind == "FloatingCast"){
        // float -> double
        expr_value = ir_builder.CreateFPCast(expr_value, to_type);
    }
    else if(cast_kind == "LValueToRValue"){
        if (!expr_value->getType()->isPointerTy() || is_global_var_decl)
        {
            // 对于非数组、指针类型的变量，可以直接返回值，无需再创建多余的load
            // 全局变量如果使用全局变量初始化，则用于初始化的全局变量必为const，json输出保留LValueToRValue，clang输出将常量折叠了
            return expr_value;
        }
        expr_value = ir_builder.CreateLoad(expr_value);
    }
    else if(cast_kind == "ArrayToPointerDecay"){
        // // debug_out("ArrayToPointerDecay");
        // if(auto dr = dynamic_cast<DeclRefExpr*>(ptr->sub_expr)){
        //     // if(dr->ref->name=="imgIn"){
        //     if(dr->ref->name=="g"){
        //         // debug_out("imgIn");
        //         debug_out(expr_value->getName().str());
        //         if(expr_value->getType()->isPointerTy()){
        //             debug_out("isp");
        //             // [2 x [3 x i32]]*
        //             llvm::errs()<<*(expr_value->getType());
        //         }
        //     } 
        // }
        // // [2 x [3 x i32]]*
        // llvm::errs()<<*(expr_value->getType())<<'\n';
        expr_value = ir_builder.CreateInBoundsGEP(expr_value, {ir_builder.getInt64(0), ir_builder.getInt64(0)});
        // expr_value = ir_builder.CreateInBoundsGEP(expr_value->getType(), expr_value, {ir_builder.getInt64(0), ir_builder.getInt64(0)});
        // [3 x i32]*
        // llvm::errs()<<*(expr_value->getType())<<'\n';
    }
    else if (cast_kind == "BitCast") {
        // debug_out("?");
        // llvm::errs()<<*(expr_value->getType())<<'\n';
        expr_value = ir_builder.CreateBitCast(expr_value, to_type);
        // llvm::errs()<<*(expr_value->getType())<<'\n';
    }

    return expr_value;
}

llvm::Value* EmitIR::CallExprToIR(CallExpr *ptr){
    // debug_out("CallExprToIR");
    auto function = module.getFunction(ptr->function->name);
    // void (i32*)*
    // llvm::errs()<<*(function->getType())<<'\n';
    // if(function==nullptr) debug_out("function_nullptr");
    std::vector<llvm::Value*> parms;
    for (auto it: ptr->parms)
    {
        // if(auto im=dynamic_cast<ImplicitCastExpr*>(it)) debug_out("im");
        auto val = ExprToIR(it);
        parms.emplace_back(val);
        // [3 x i32]*
        // llvm::errs()<<*(val->getType())<<'\n';
    }
    // 当函数的返回值为void时，不需要要有name
    // /workspace/SYsU-lang/tester/function_test2020/09_void_func.sysu.c
    std::string call_name = ptr->function->name;
    if(function->getReturnType()->isVoidTy()) call_name = "";
    return ir_builder.CreateCall(function, parms, call_name);
}

// --------Stmt--------
void EmitIR::CompoundStmtToIR(CompoundStmt *ptr, llvm::BasicBlock *continue_block, llvm::BasicBlock *break_block){
    // debug_out("CompoundStmtToIR");
    for (auto it: ptr->stmts)
    {
        // 注意要传入continue_block和break_block！
        StmtToIR(it, continue_block, break_block);
    }
}

void EmitIR::NullStmtToIR(NullStmt *ptr){
    // debug_out("NullStmtToIR");
    return;
}

void EmitIR::ReturnStmtToIR(ReturnStmt *ptr){
    // debug_out("ReturnStmtToIR");
    ir_builder.CreateRet(ExprToIR(ptr->return_exp));
}

void EmitIR::DeclStmtToIR(DeclStmt *ptr){
    // debug_out("DeclStmtToIR");
    for (auto it: ptr->decls)
    {
        DeclToIR(it);
    }
}

void EmitIR::ExprStmtToIR(ExprStmt *ptr){
    // debug_out("ExprStmtToIR");
    ExprToIR(ptr->expr);
}

llvm::Instruction* EmitIR::getOrCreateBr(llvm::BasicBlock* block){
    auto cur_block = ir_builder.GetInsertBlock();
    if(cur_block->empty()) return ir_builder.CreateBr(block);

    auto& last_inst = cur_block->back();
    if(!llvm::isa<llvm::ReturnInst>(last_inst) && !llvm::isa<llvm::BranchInst>(last_inst))
        return ir_builder.CreateBr(block);

    return nullptr;
}

void EmitIR::IfStmtToIR(IfStmt *ptr, llvm::BasicBlock *continue_block, llvm::BasicBlock *break_block){
    // debug_out("IfStmtToIR");
    // 对于逻辑与或运算会增加block，因此需要提前计算condition的值
    auto condition_value = ValueToBool(ExprToIR(ptr->condition));
    auto cur_block = ir_builder.GetInsertBlock();
    auto function = cur_block->getParent();
    auto if_then = llvm::BasicBlock::Create(context, "if.then", function);
    llvm::BasicBlock * if_else = nullptr;
    auto if_end = llvm::BasicBlock::Create(context, "if.end");
    // cur_block
    // if(ptr->else_stmt==nullptr) debug_out("else_nullptr");
    if(ptr->else_stmt != nullptr){
        if_else = llvm::BasicBlock::Create(context, "if.else");
        ir_builder.CreateCondBr(condition_value, if_then, if_else);
    }
    else ir_builder.CreateCondBr(condition_value, if_then, if_end);

    // if_then
    ir_builder.SetInsertPoint(if_then);
    // if(break_block==nullptr) debug_out("nullptr_block");
    // if(break_block!=nullptr) debug_out(break_block->getName().str());
    StmtToIR(ptr->if_stmt, continue_block, break_block);
    getOrCreateBr(if_end);

    // else_block
    if (if_else != nullptr)
    {
        function->getBasicBlockList().push_back(if_else);
        ir_builder.SetInsertPoint(if_else);
        StmtToIR(ptr->else_stmt, continue_block, break_block);
        getOrCreateBr(if_end);
    }
    
    // if_end
    function->getBasicBlockList().push_back(if_end);
    ir_builder.SetInsertPoint(if_end);
}

void EmitIR::WhileStmtToIR(WhileStmt *ptr){
    // debug_out("WhileStmtToIR");
    auto cur_block = ir_builder.GetInsertBlock();
    auto function = cur_block->getParent();
    auto while_cond = llvm::BasicBlock::Create(context, "while.cond", function);
    auto while_body = llvm::BasicBlock::Create(context, "while.body");
    auto while_end = llvm::BasicBlock::Create(context, "while.end");
    // cur_block
    ir_builder.CreateBr(while_cond);

    // while_cond
    ir_builder.SetInsertPoint(while_cond);
    auto condition_value = ValueToBool(ExprToIR(ptr->condition));
    ir_builder.CreateCondBr(condition_value, while_body, while_end);

    // body_block
    function->getBasicBlockList().push_back(while_body);
    ir_builder.SetInsertPoint(while_body);
    // debug_out(while_end->getName().str());
    StmtToIR(ptr->stmt, while_cond, while_end);
    // 在while body中有可能会产生br或ret
    // /workspace/SYsU-lang/tester/function_test2020/10_break.sysu.c
    getOrCreateBr(while_cond);
    
    // end_block
    function->getBasicBlockList().push_back(while_end);
    ir_builder.SetInsertPoint(while_end);
}   

void EmitIR::DoStmtToIR(DoStmt *ptr){
    // debug_out("DoStmtToIR");
    auto cur_block = ir_builder.GetInsertBlock();
    auto function = cur_block->getParent();
    auto body_block = llvm::BasicBlock::Create(context, "do.body", function);
    auto condition_block = llvm::BasicBlock::Create(context, "do.condition");
    auto end_block = llvm::BasicBlock::Create(context, "do.end");
    // cur_block
    ir_builder.CreateBr(body_block);

    // body_block
    ir_builder.SetInsertPoint(body_block);
    StmtToIR(ptr->stmt, condition_block, end_block);
    // 在do body中有可能会产生br或ret
    getOrCreateBr(condition_block);

    // condition_block
    function->getBasicBlockList().push_back(condition_block);
    ir_builder.SetInsertPoint(condition_block);
    auto condition_value = ValueToBool(ExprToIR(ptr->condition));
    ir_builder.CreateCondBr(condition_value, body_block, end_block);
    
    // end_block
    function->getBasicBlockList().push_back(end_block);
    ir_builder.SetInsertPoint(end_block);
}

void EmitIR::BreakStmtToIR(BreakStmt *ptr, llvm::BasicBlock *break_block){
    // debug_out("BreakStmtToIR");
    ir_builder.CreateBr(break_block);
}

void EmitIR::ContinueStmtToIR(ContinueStmt *ptr, llvm::BasicBlock *continue_block){
    // debug_out("ContinueStmtToIR");
    ir_builder.CreateBr(continue_block);
}

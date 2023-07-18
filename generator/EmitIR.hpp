#ifndef EMITIR_HPP
#define EMITIR_HPP

#include "ASG.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <unordered_map>

// extern void debug_out(std::string str);

extern llvm::LLVMContext context;
extern llvm::Module module;

class EmitIR
{
public:
    llvm::IRBuilder<> ir_builder;
    std::unordered_map<Obj*, llvm::AllocaInst*> symbol_table;
    bool is_global_var_decl;

    EmitIR(): ir_builder(context), is_global_var_decl(false)
    {symbol_table.clear();}
    ~EmitIR() = default;

    // TranslationUnitDecl
    void TranslationUnitDeclToIR(TranslationUnitDecl *root);

    // 基类
    void DeclToIR(Decl *ptr);
    llvm::Value* ExprToIR(Expr *ptr, llvm::Value *addr = nullptr);
    void StmtToIR(Stmt *ptr, llvm::BasicBlock *continue_block = nullptr, llvm::BasicBlock *break_block = nullptr);

    // Decl
    void GlobalVarDeclToIR(VarDecl* ptr);
    void VarDeclToIR(VarDecl *ptr);
    void ParmVarDeclToIR(ParmVarDecl *ptr);
    void FunctionDeclToIR(FunctionDecl *ptr);

    // Expr
    llvm::Value* IntegerLiteralToIR(IntegerLiteral *ptr);
    llvm::Value* FloatingLiteralToIR(FloatingLiteral *ptr);
    llvm::Value* CharacterLiteralToIR(CharacterLiteral *ptr);
    llvm::Value* StringLiteralToIR(StringLiteral *ptr);
    llvm::Value* UnaryExprToIR(UnaryExpr *ptr);
    llvm::Value* BinaryExprToIR(BinaryExpr *ptr);
    llvm::Value* ANDExprToIR(BinaryExpr *ptr);
    llvm::Value* ORExprToIR(BinaryExpr *ptr);
    llvm::Value* ParenExprToIR(ParenExpr *ptr);
    llvm::Value* DeclRefExprToIR(DeclRefExpr *ptr);
    llvm::Value* InitListExprToIR(InitListExpr *ptr, llvm::Value *addr = nullptr);
    llvm::Value* ArraySubscriptExprToIR(ArraySubscriptExpr *ptr);
    llvm::Value* ImplicitCastExprToIR(ImplicitCastExpr *ptr);
    llvm::Value* CallExprToIR(CallExpr *ptr);

    // Stmt
    void CompoundStmtToIR(CompoundStmt *ptr, llvm::BasicBlock *continue_block, llvm::BasicBlock *break_block);
    void NullStmtToIR(NullStmt *ptr);
    void ReturnStmtToIR(ReturnStmt *ptr);
    void DeclStmtToIR(DeclStmt *ptr);
    void ExprStmtToIR(ExprStmt *ptr);
    void IfStmtToIR(IfStmt *ptr, llvm::BasicBlock *continue_block, llvm::BasicBlock *break_block);
    void WhileStmtToIR(WhileStmt *ptr);
    void DoStmtToIR(DoStmt *ptr);
    void BreakStmtToIR(BreakStmt *ptr, llvm::BasicBlock *break_block);
    void ContinueStmtToIR(ContinueStmt *ptr, llvm::BasicBlock *continue_block);

    // 辅助函数
    void registerPtr(Obj *ptr, llvm::AllocaInst *alloca_inst);
    llvm::Value* ValueToBool(llvm::Value* value);
    llvm::Instruction* getOrCreateBr(llvm::BasicBlock* block);
};

#endif
#ifndef JSONTOASG_HPP
#define JSONTOASG_HPP

#include "ASG.hpp"
#include <unordered_map>
#include <llvm/Support/JSON.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

// extern void debug_out(std::string str);

class JsonToASG
{
public:
    llvm::LLVMContext *context;
    std::unordered_map<int, Decl*> symbol_table;

    JsonToASG() = default;
    ~JsonToASG() = default;

    // TranslationUnitDecl
    TranslationUnitDecl* TranslationUnitDeclToASG(llvm::json::Object* object, llvm::LLVMContext *context);

    // Type
    Type getType(llvm::json::Object* object);
    llvm::Type* getBasicType(const std::string& basic_type_str);
    llvm::Type* getPtrType(llvm::Type* basic_type, const std::string& type_str);
    llvm::Type* getArrayType(llvm::Type* element_type, const std::string& type_str);
    llvm::FunctionType* getFunctionType(llvm::json::Object* object);
    llvm::Type* getStrType(const std::string& type_str);

    Obj* ObjToASG(llvm::json::Object* object);

    // 基类
    Decl* DeclToASG(llvm::json::Object* object);
    Expr* ExprToASG(llvm::json::Object* object);
    Stmt* StmtToASG(llvm::json::Object* object);

    // Decl
    VarDecl* VarDeclToASG(llvm::json::Object* object);
    ParmVarDecl* ParmVarDeclToASG(llvm::json::Object* object);
    FunctionDecl* FunctionDeclToASG(llvm::json::Object* object);

    // Expr
    IntegerLiteral* IntegerLiteralToASG(llvm::json::Object* object);
    FloatingLiteral* FloatingLiteralToASG(llvm::json::Object* object);
    CharacterLiteral* CharacterLiteralToASG(llvm::json::Object* object);
    StringLiteral* StringLiteralToASG(llvm::json::Object* object);
    UnaryExpr* UnaryExprToASG(llvm::json::Object* object);
    BinaryExpr* BinaryExprToASG(llvm::json::Object* object);
    ParenExpr* ParenExprToASG(llvm::json::Object* object);
    DeclRefExpr* DeclRefExprToASG(llvm::json::Object* object);
    InitListExpr* InitListExprToASG(llvm::json::Object* object);
    ArraySubscriptExpr* ArraySubscriptExprToASG(llvm::json::Object* object);
    ImplicitCastExpr* ImplicitCastExprToASG(llvm::json::Object* object);
    CallExpr* CallExprToASG(llvm::json::Object* object);

    // Stmt
    CompoundStmt* CompoundStmtToASG(llvm::json::Object* object);
    NullStmt* NullStmtToASG(llvm::json::Object* object);
    ReturnStmt* ReturnStmtToASG(llvm::json::Object* object);
    DeclStmt* DeclStmtToASG(llvm::json::Object* object);
    ExprStmt* ExprStmtToASG(llvm::json::Object* object);
    IfStmt* IfStmtToASG(llvm::json::Object* object);
    WhileStmt* WhileStmtToASG(llvm::json::Object* object);
    DoStmt* DoStmtToASG(llvm::json::Object* object);
    BreakStmt* BreakStmtToASG(llvm::json::Object* object);
    ContinueStmt* ContinueStmtToASG(llvm::json::Object* object);
    
    // 辅助函数
    std::string getName(llvm::json::Object* object);
    void registerID(llvm::json::Object* object, Decl* decl);
    void LogoutID(llvm::json::Object* object);
    Decl* findRef(llvm::json::Object* object);
};

#endif
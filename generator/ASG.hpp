#ifndef ASG_HPP
#define ASG_HPP

#include "llvm/IR/Type.h"
#include <llvm/IR/DerivedTypes.h>
#include <vector>

// Type
struct Type
{
    llvm::Type *type;
    bool is_const;

    Type(llvm::Type *type = nullptr, bool is_const = false): 
        type(type), is_const(is_const){}
};

struct Obj
{
    virtual ~Obj() = default;
};

// 基类
struct Decl: public Obj
{
    Type type;
    std::string name;
    bool is_promotable;

    Decl(): type(Type()), name(std::string("")), is_promotable(true){}
    Decl(const Type& type, const std::string& name, bool is_promotable = true): 
        type(type), name(name), is_promotable(is_promotable){}
};

struct Expr: public Obj
{
    Type type;

    Expr(): type(Type()){}
    Expr(const Type& type): type(type){}
};

struct Stmt: public Obj{};

// --------Decl--------
struct VarDecl: public Decl
{
    Expr *init_expr;

    VarDecl(): Decl(Decl()), init_expr(nullptr){}
    VarDecl(const Type& type, const std::string& name, Expr *init_expr = nullptr): 
        Decl(type, name), init_expr(init_expr){}
};

struct ParmVarDecl: public Decl
{
    ParmVarDecl(): Decl(Decl()){}
    ParmVarDecl(const Type& type, const std::string& name): Decl(type, name){}
};

struct FunctionDecl: public Decl
{
    llvm::FunctionType *function_type;
    std::vector<ParmVarDecl*> parm_var_decls;
    Stmt *block;

    FunctionDecl(): Decl(Decl()), function_type(nullptr), block(nullptr){
        parm_var_decls.clear();
    }
    FunctionDecl(llvm::FunctionType *function_type, const std::string& name, const std::vector<ParmVarDecl*>& parm_var_decls, Stmt *block = nullptr):
        Decl(Type(), name), function_type(function_type), parm_var_decls(parm_var_decls), block(block){}
};

// --------Expr--------
// Literal
struct IntegerLiteral: public Expr
{
    int value;

    IntegerLiteral(): Expr(Expr()){}
    IntegerLiteral(const Type& type, unsigned long long value):
        Expr(type), value(value){}
};

struct FloatingLiteral: public Expr
{
    double value;

    FloatingLiteral(): Expr(Expr()){}
    FloatingLiteral(const Type& type, double value):
        Expr(type), value(value){}
};

struct CharacterLiteral: public Expr
{
    char value;

    CharacterLiteral(): Expr(Expr()){}
    CharacterLiteral(const Type& type, char value):
        Expr(type), value(value){}
};

struct StringLiteral: public Expr
{
    std::string value;

    StringLiteral(): Expr(Expr()), value(std::string("")){}
    StringLiteral(const Type& type, const std::string& value):
        Expr(type), value(value){}
};

// Op
struct UnaryExpr: public Expr
{
    std::string opcode;
    Expr *expr;

    UnaryExpr(): Expr(Expr()), opcode(std::string("")), expr(nullptr){}
    UnaryExpr(const Type& type, const std::string& opcode, Expr *expr):
        Expr(type), opcode(opcode), expr(expr){}
};

struct BinaryExpr: public Expr
{
    std::string opcode;
    Expr *lhs;
    Expr *rhs;

    BinaryExpr(): Expr(Expr()), opcode(std::string("")), lhs(nullptr), rhs(nullptr){}
    BinaryExpr(const Type& type, const std::string& opcode, Expr *lhs, Expr *rhs):
        Expr(type), opcode(opcode), lhs(lhs), rhs(rhs){}
};

// ()
struct ParenExpr: public Expr
{
    Expr *expr;

    ParenExpr(): Expr(Expr()), expr(nullptr){}
    ParenExpr(const Type& type, Expr *expr = nullptr):
        Expr(type), expr(expr){}
};

// Included in stmt
// DeclStmt
struct DeclRefExpr: public Expr
{
    Decl *ref;

    DeclRefExpr(): Expr(Type()), ref(nullptr){}
    DeclRefExpr(const Type& type, Decl *ref):
        Expr(type), ref(ref){}
};

// init
struct InitListExpr: public Expr
{
    std::vector<Expr*> sub_init_exprs;

    InitListExpr(): Expr(Expr()){sub_init_exprs.clear();}
    InitListExpr(const Type& type, const std::vector<Expr*>& sub_init_exprs):
        Expr(type), sub_init_exprs(sub_init_exprs){}
};

// ArraySubscriptExpr
struct ArraySubscriptExpr: public Expr
{
    Expr *sub_expr;
    std::vector<Expr*> indexs;

    ArraySubscriptExpr(): Expr(Expr()), sub_expr(nullptr){indexs.clear();} 
    ArraySubscriptExpr(const Type& type, Expr *sub_expr, const std::vector<Expr*>& indexs):
        Expr(type), sub_expr(sub_expr), indexs(indexs){}
};

// ImplicitCastExpr
struct ImplicitCastExpr: public Expr
{
    std::string cast_kind;
    Expr *sub_expr;

    ImplicitCastExpr(): Expr(Expr()), cast_kind(std::string("")), sub_expr(nullptr){}
    ImplicitCastExpr(const Type& type, const std::string& cast_kind, Expr *sub_expr = nullptr):
        Expr(type), cast_kind(cast_kind), sub_expr(sub_expr){}
};

struct CallExpr: public Expr
{
    Decl *function;
    std::vector<Expr*> parms;
    
    CallExpr(): Expr(Expr()), function(nullptr){parms.clear();}
    CallExpr(const Type& type, Decl *function, const std::vector<Expr*>& parms):
        Expr(type), function(function), parms(parms){}
};

// --------Stmt--------
struct CompoundStmt: public Stmt
{
    std::vector<Stmt*> stmts;

    CompoundStmt(){stmts.clear();}
    CompoundStmt(const std::vector<Stmt*>& stmts): stmts(stmts){}
};

struct NullStmt : public Stmt{};

struct ReturnStmt: public Stmt
{
    Expr *return_exp;

    ReturnStmt(Expr *return_exp = nullptr): return_exp(return_exp){}
};

struct DeclStmt: public Stmt
{
    std::vector<Decl*> decls;

    DeclStmt(){decls.clear();}
    DeclStmt(const std::vector<Decl*>& decls): decls(decls){}
};

struct ExprStmt: public Stmt
{
    Expr *expr;

    ExprStmt(Expr *expr = nullptr): expr(expr){}
};

struct IfStmt: public Stmt
{
    Expr *condition;
    Stmt *if_stmt;
    Stmt *else_stmt;

    IfStmt(Expr *condition = nullptr, Stmt *if_stmt = nullptr, Stmt *else_stmt = nullptr):
        condition(condition), if_stmt(if_stmt), else_stmt(else_stmt){}
};

struct WhileStmt: public Stmt
{
    Expr *condition;
    Stmt *stmt;

    WhileStmt(Expr *condition = nullptr, Stmt* stmt = nullptr):
        condition(condition), stmt(stmt){}
}; 

struct DoStmt: public Stmt
{
    Expr *condition;
    Stmt *stmt;

    DoStmt(Expr *condition = nullptr, Stmt* stmt = nullptr):
        condition(condition), stmt(stmt){}    
};

struct BreakStmt: public Stmt{};

struct ContinueStmt : public Stmt{};

// --------TranslationUnitDecl--------
struct TranslationUnitDecl: public Obj
{
    std::vector<VarDecl*> global_var_decls;
    std::vector<FunctionDecl*> function_decls;

    TranslationUnitDecl(){
        global_var_decls.clear();
        function_decls.clear();
    }
};

#endif
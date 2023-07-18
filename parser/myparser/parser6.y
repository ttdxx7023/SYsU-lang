%code requires
{
class Tree;
}
%{
#include "parser.hh"
#include <vector>
#include <map>
#include <memory>
#include <llvm/Support/JSON.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <llvm/ADT/APFloat.h>

#define yyerror(x)                                                             \
  do {                                                                         \
    llvm::errs() << (x);                                                       \
  } while (0)

namespace {
auto llvmin = llvm::MemoryBuffer::getFileOrSTDIN("-");
auto input = llvmin.get() -> getBuffer();
auto end = input.end(), it = input.begin();
auto wk_getline(char endline = "\n"[0]) {
  auto beg = it;
  while (it != end && *it != endline)
    ++it;
  auto len = it - beg;
  if (it != end && *it == endline)
    ++it;
  return llvm::StringRef(beg, len);
}
Tree* root;
} // namespace

// 以下树结构仅供参考，你可以随意修改或定义自己喜欢的形式
class Tree{
public:
  std::string kind;
  std::string name;
  std::string value;
  std::string type;
  std::vector<std::unique_ptr<Tree>> sons;
  std::vector<std::unique_ptr<Tree>> brothers;
  Tree(std::string kind="", std::string name="", std::string value="", std::string type="int"): kind(kind), name(name), value(value), type(type) {}
  void addSon(Tree* son){ sons.emplace_back(std::unique_ptr<Tree>(son)); }
  void addSon(std::unique_ptr<Tree>&& son){ sons.emplace_back(std::move(son)); }
  void addBrother(Tree* brother){ brothers.emplace_back(std::unique_ptr<Tree>(brother)); }
  void addBrother(std::unique_ptr<Tree>&& brother){ brothers.emplace_back(std::move(brother)); }
  void addSons(Tree* son){
    for(auto it = son->brothers.begin(); it != son->brothers.end(); it++){
      addSon(std::move(*it));
    }
  }
  void addBrothers(Tree* brother){
      for(auto it = brother->brothers.begin(); it != brother->brothers.end(); it++){
      addBrother(std::move(*it));
    }
  }
  llvm::json::Value toJson() const {
    llvm::json::Object tmp{
      {"kind", kind},
      {"name", name},
      {"value", value},
      {"type", type},
      {"inner", llvm::json::Array{}}
    };
    for(auto&& it: sons) tmp.get("inner")->getAsArray()->push_back(it->toJson());
    return tmp;
  }
  void print(int depth=0) const {
    yyerror("|");
    for(int i=0;i<depth;++i) yyerror(" ");
    yyerror("-"+kind+" "+name+" "+value+" "+type);
    for(auto&& it: sons)
    {
      yyerror("\n");
      it->print(depth+1);
    }
    if(!depth) yyerror("\n\n");
  }
  void DFS(std::string& ret_type){
    // std::cout<<kind<<"1\n";
    if(kind == "ReturnStmt"){
      // std::cout<<kind<<"2\n";
      // 返回语句
      if(type != ret_type){
        // 存在隐式转换
        type = ret_type; // 保证不再处理第二次
        auto ptr = new Tree("ImplicitCastExpr", "", "", ret_type);
        ptr->sons.swap(sons);
        addSon(ptr);
      }
    }
    if(sons.empty()){
      // std::cout<<kind<<"3\n";
      return;
    }
    for(auto it = sons.begin(); it != sons.end(); it++){
      // std::cout<<(*it)->kind<<"4\n";
      // 剪枝
      // if(it->kind == "CompoundStmt"){
        (*it)->DFS(ret_type);
      // }
    }
    return;
  }
};
// 符号表
llvm::json::Object SymTable;
void add_sym(std::string name, std::string type){
  if(SymTable.get(name) != nullptr){
    SymTable.erase(name);
  }
  SymTable.insert({name, type});
}
std::string get_sym_type(std::string name){
  // 查找符号表并返回类型，默认为int
  std::string type = "int";
  if(SymTable.get(name) != nullptr){
    type = SymTable.getString(name)->str();
  }
  return type;
}
int getPriority(std::string &type) {
  if (type == "int")
    return 0;
  if (type == "unsigned int")
    return 1;
  if (type == "long")
    return 2;
  if (type == "float")
    return 3;
  if (type == "double")
    return 4;
  if (type == "long long")
    return 5;
  return 0;
}
void BinaryOpImplicitCast(Tree* op, Tree* l, Tree* r){
  Tree* lson = l;
  Tree* rson = r;
  std::string optype = l->type;
  if(l->type != r->type){
    // 存在隐式转换
    std::string change;
    auto ptr = new Tree("ImplicitCastExpr");
    if(getPriority(l->type) > getPriority(r->type)){
      optype = l->type;
      change = r->type + "to" + l->type;
      ptr->name = change;
      ptr->type = optype;
      ptr->addSon(r);
      rson = ptr;
    }
    else{
      optype = r->type;
      change = l->type + "to" + r->type;
      ptr->name = change;
      ptr->type = optype;
      ptr->addSon(l);
      lson = ptr;
    }
  }
  op->type = optype;
  op->addSon(lson);
  op->addSon(rson);
}

auto yylex() {
  auto tk = wk_getline();
  auto b = tk.find("'") + 1, e = tk.rfind("'");
  auto s = tk.substr(b, e - b), t = tk.substr(0, tk.find(" "));
  if (t == "numeric_constant") {
    // 检测是否是浮点数，p计数法浮点数，e计数法浮点数，如果是，转换成double
    if (s.find('.') != std::string::npos || s.find('p') != std::string::npos ||
        s.find('e') != std::string::npos) {
      yylval = new Tree("FloatingLiteral", "", s.str(), "double");
      llvm::StringRef str(yylval->value);
      llvm::APFloat apf(0.0);
      apf.convertFromString(str, llvm::APFloat::rmNearestTiesToEven);
      llvm::SmallString<16> Buffer;
      apf.toString(Buffer);
      yylval->value = Buffer.c_str();
      return T_NUMERIC_CONSTANT;
    }
    else{
      long num;
      s.getAsInteger(0, num);
      std::string t;
      if(num <= 0x7fFffffF){
        t = "int";
      }
      else if(num <= 0xffffffff){
        t = "unsigned int";
      }
      else{
        t = "long";
      }
      yylval = new Tree("IntegerLiteral", "", std::to_string(num), t);
      return T_NUMERIC_CONSTANT;
    }
  }
  if (t == "identifier") {
    yylval = new Tree("id", s.str());
    return T_IDENTIFIER;
  }
  if (t == "int")
    return T_INT;
  if (t == "return")
    return T_RETURN;
  if (t == "semi")
    return T_SEMI;
  if (t == "l_paren")
    return T_L_PAREN;
  if (t == "r_paren")
    return T_R_PAREN;
  if (t == "l_brace")
    return T_L_BRACE;
  if (t == "r_brace")
    return T_R_BRACE;
// TO-DO：你需要在这里补充更多的TOKEN
  if (t == "plus")
    return T_PLUS;
  if (t == "minus")
    return T_MINUS;
  if (t == "star")
    return T_STAR;
  if (t == "slash")
    return T_SLASH;
  if (t == "percent")
    return T_PERCENT;
  if (t == "const")
    return T_CONST;
  if (t == "comma")
    return T_COMMA;
  if (t == "equal")
    return T_EQUAL;
  if (t == "if")
    return T_IF;
  if (t == "else")
    return T_ELSE;
  if (t == "exclaim")
    return T_EXCLAIM;
  if (t == "greater")
    return T_GREATER;
  if (t == "greaterequal")
    return T_GREATEREQUAL;
  if (t == "equalequal")
    return T_EQUALEQUAL;
  if (t == "exclaimequal")
    return T_EXCLAIMEQUAL;
  if (t == "less")
    return T_LESS;
  if (t == "lessequal")
    return T_LESSEQUAL;
  if (t == "pipepipe")
    return T_PIPEPIPE;
  if (t == "ampamp")
    return T_AMPAMP;
  if (t == "while")
    return T_WHILE;
  if (t == "break")
    return T_BREAK;
  if (t == "continue")
    return T_CONTINUE;
  if (t == "l_square")
    return T_L_SQUARE;
  if (t == "r_square")
    return T_R_SQUARE;
  if (t == "void")
    return T_VOID;
  if (t == "float")
    return T_FLOAT;
  if (t == "double")
    return T_DOUBLE;
  if (t == "long")
    return T_LONG;
  if (t == "char")
    return T_CHAR;
  if (t == "ellipsis")
    return T_ELLIPSIS;
  if (t == "do")
    return T_DO;
  if (t == "string_literal") {
    yylval = new Tree("StringLiteral", "", s.str(), "char");
    return T_STRING;
  }
  return YYEOF;
}

int main() {
  yyparse();
  root->print();
  llvm::outs() << root->toJson() << "\n";
}
%}
%define api.value.type { Tree* }

// TO-DO：你需要在这里补充更多的TOKEN
%token T_NUMERIC_CONSTANT
%token T_IDENTIFIER
%token T_INT
%token T_RETURN
%token T_SEMI
%token T_L_PAREN
%token T_R_PAREN
%token T_L_BRACE
%token T_R_BRACE
%token T_PLUS
%token T_MINUS
%token T_STAR
%token T_SLASH
%token T_PERCENT
%token T_CONST
%token T_COMMA
%token T_EQUAL
%token T_IF
%token T_ELSE
%token T_EXCLAIM
%token T_GREATER
%token T_GREATEREQUAL
%token T_EQUALEQUAL
%token T_EXCLAIMEQUAL
%token T_LESS
%token T_LESSEQUAL
%token T_PIPEPIPE
%token T_AMPAMP
%token T_WHILE
%token T_BREAK
%token T_CONTINUE
%token T_L_SQUARE
%token T_R_SQUARE
%token T_VOID
%token T_FLOAT
%token T_DOUBLE
%token T_LONG
%token T_CHAR
%token T_ELLIPSIS
%token T_DO
%token T_STRING
%right IF_THEN T_ELSE
%right PrimLVal T_R_PAREN
%start Begin
%%
Begin: CompUnit {
  auto ptr = new Tree("TranslationUnitDecl", "", "", "");
  ptr->addSon($1);
  if(!$1->brothers.empty()){
    ptr->addSons($1);
  }
  root = ptr;
}
;

CompUnit: CompUnit GlobalDecl {
  $1->addBrother($2);
  if(!$2->brothers.empty()){
    $1->addBrothers($2);
  }
  $$ = $1;
}
| GlobalDecl {

}
;

GlobalDecl: FuncDef {
  $$ = $1;
}
| VarDecl {
  $$ = $1;
}
| FuncDecl {

}
;

VarDecl: BType VarDef T_SEMI {
  std::string ltype = $1->type;
  // 检查是否存在隐式转换
  if($2->type == "id"){
      if(!$2->sons.empty()){
      std::string rtype = $2->sons[0]->type;
      if(ltype != rtype){
        std::string change = rtype + "to" + ltype;
        auto ptr = new Tree("ImplicitCastExpr", change, "", ltype);
        ptr->sons.swap($2->sons);
        $2->addSon(ptr);
      }
    }
  }
  else if($2->type == "array"){
    // TODO
    // 数组类型
    // if(!$2->sons.empty()){
    //   auto InitListExprPtr = std::move($2->sons[0]);
    //   if(!InitListExprPtr->sons.empty()){
    //     // 存在初始化
    //     for(auto it = InitListExpr->sons.begin(); it != InitListExprPtr->sons.end(); it++){
    //       std::string rtype = (*it)->type;
    //       if(ltype != rtype){
    //         std::string change = rtype + "to" + ltype;
    //         auto imp = new Tree("ImplicitCastExpr", change, "", ltype);
    //       }
    //     }
    //   }
    //   else{
    //     // 不存在初始化
    //   }
    // }
  }
  if(!$2->brothers.empty()){
    for(auto it = $2->brothers.begin(); it != $2->brothers.end(); it++){
      if(!(*it)->sons.empty() && (*it)->type == "id"){
        std::string rtype = (*it)->sons[0]->type;
        if(ltype != rtype){
          std::string change = rtype + "to" + ltype;
          auto ptr = new Tree("ImplicitCastExpr", change, "", ltype);
          ptr->sons.swap((*it)->sons);
          (*it)->addSon(ptr);
        }
      }
      else if((*it)->type == "array"){
        // TODO
      }
    }
  } 
  // 将type加入var
  $2->type = ltype;
  if(!$2->brothers.empty()){
    for(auto it = $2->brothers.begin(); it != $2->brothers.end(); it++){
      (*it)->type = ltype;
    }
  }
  // 将var加入符号表
  add_sym($2->name, $2->type);
  if(!$2->brothers.empty()){
    for(auto it = $2->brothers.begin(); it != $2->brothers.end(); it++){
      add_sym((*it)->name, (*it)->type);
    }
  }
  delete $1;
  $$ = $2;
}

BType: T_INT {
  auto ptr = new Tree("", "", "", "int");
  $$ = ptr;
}
| T_CONST T_INT {
  auto ptr = new Tree("", "", "", "int");
  $$ = ptr;
}
| T_VOID {
  auto ptr = new Tree("", "", "", "void");
  $$ = ptr;
}
| T_INT T_STAR {
  auto ptr = new Tree("", "", "", "int *");
  $$ = ptr;
}
| T_FLOAT {
  auto ptr = new Tree("", "", "", "float");
  $$ = ptr;
}
| T_CONST T_FLOAT {
  auto ptr = new Tree("", "", "", "float");
  $$ = ptr;
}
| T_DOUBLE {
  auto ptr = new Tree("", "", "", "double");
  $$ = ptr;
}
| T_CONST T_DOUBLE {
  auto ptr = new Tree("", "", "", "double");
  $$ = ptr;
}
| T_LONG {
  auto ptr = new Tree("", "", "", "long");
  $$ = ptr;
}
| T_CONST T_LONG {
  auto ptr = new Tree("", "", "", "long");
  $$ = ptr;
}
| T_CHAR {
  auto ptr = new Tree("", "", "", "char");
  $$ = ptr;
}
| T_CONST T_CHAR {
  auto ptr = new Tree("", "", "", "char");
  $$ = ptr;
}
| T_LONG T_LONG {
  auto ptr = new Tree("", "", "", "long long");
  $$ = ptr;
}
;

VarDef: VarDefUnit {

}
| VarDef T_COMMA VarDefUnit {
  $1->addBrother($3);
  $$ = $1;
}
;

VarDefUnit: T_IDENTIFIER {
  auto ptr = new Tree("VarDecl", $1->name, "", "id");
  delete $1;
  $$ = ptr;
}
| T_IDENTIFIER T_EQUAL InitVal {
  auto ptr = new Tree("VarDecl", $1->name, "", "id");
  delete $1;
  ptr->addSon($3);
  $$ = ptr;
}
| T_IDENTIFIER MultiDimensional {
  auto ptr = new Tree("VarDecl", $1->name, $2->value, "array");
  delete $1;
  // delete $2;
  $$ = ptr;
}
| T_IDENTIFIER MultiDimensional T_EQUAL InitVal {
  auto ptr = new Tree("VarDecl", $1->name, $2->value, "array");
  if($2->kind == "ArraySize"){
    int array_size = std::stoi(ptr->value);
    if(array_size != $4->sons.size()){
      auto filler_ptr = new Tree("array_filler");
      $4->sons.emplace($4->sons.begin(), std::move(filler_ptr));
    }
  }
  ptr->addSon($4);
  delete $1;
  // delete $2;
  $$ = ptr;
}
;

OneDimension: T_L_SQUARE Exp T_R_SQUARE {
  if($2->kind == "IntegerLiteral"){
    auto ptr = new Tree("ArraySize", "", $2->value, "");
    $$ = ptr;
  }
}
;

MultiDimensional: OneDimension {

}
| MultiDimensional OneDimension{
  
}
;

InitVal: Exp {

}
| T_L_BRACE InitValList T_R_BRACE {
// | InitValList {
  int init_cnt = 0;
  auto ptr = new Tree("InitListExpr", "", "", $2->type);
  ptr->addSon($2);
  if(!$2->brothers.empty()){
    ptr->addSons($2);
  }
  $$ = ptr;
}
| T_L_BRACE T_R_BRACE {
  auto ptr = new Tree("InitListExpr");
  auto son = new Tree("array_filler");
  ptr->addSon(son);
  $$ = ptr;
}
;

InitValList: InitVal {
  // int a[1] = {1};
}
| InitValList T_COMMA InitVal {
  // int a[2] = {1, 2};
  $1->addBrother($3);
  $$ = $1;
}
// | T_L_BRACE InitValList T_R_BRACE {
//   // int a[2][2] = {{1,2}};
// }
// | InitValList T_COMMA InitValList {
//   // int a[2][2] = {{1,2}, {3,4}};
//   $1->addBrother($3);
//   $$ = $1;
// }
// | {
//   // int a[4][2] = {};
// }
// ;



FuncDef:BType T_IDENTIFIER T_L_PAREN T_R_PAREN Block {
  auto ptr = new Tree("FunctionDecl", $2->name);
  // 函数返回值的隐式转换
  $5->DFS($1->type);
  ptr->type = $1->type + " ()";
  ptr->addSon($5);
  add_sym(ptr->name, ptr->type);
  delete $1;
  delete $2;
  $$ = ptr;
}
| BType T_IDENTIFIER T_L_PAREN FuncFParams T_R_PAREN Block {
  // 注意不是FuncRParams，区别为R、F。
  auto ptr = new Tree("FunctionDecl", $2->name, "", "");
  // 函数返回值的隐式转换
  $6->DFS($1->type);
  // type为"返回值type (参数列表type)"
  ptr->type = $1->type + " (";
  ptr->type += $4->type;
  if(!$4->brothers.empty()){
    for(auto it = $4->brothers.begin(); it != $4->brothers.end(); it++){
      ptr->type += ",";
      ptr->type += (*it)->type;
    }
  }
  ptr->type += ")";
  add_sym(ptr->name, ptr->type);
  ptr->addSon($4);
  if(!$4->brothers.empty()){
    ptr->addSons($4);
  }
  ptr->addSon($6);
  delete $1;
  delete $2;
  $$ = ptr;
}
;

FuncDecl:BType T_IDENTIFIER T_L_PAREN T_R_PAREN T_SEMI {
  auto ptr = new Tree("FunctionDecl", $2->name);
  ptr->type = $1->type + " ()";
  add_sym(ptr->name, ptr->type);
  delete $1;
  delete $2;
  $$ = ptr;
}
| BType T_IDENTIFIER T_L_PAREN FuncFParams T_R_PAREN T_SEMI {
  // 注意不是FuncRParams，区别为R、F。
  auto ptr = new Tree("FunctionDecl", $2->name, "", "");
  // type为"返回值type (参数列表type)"
  ptr->type = $1->type + " (";
  ptr->type += $4->type;
  if(!$4->brothers.empty()){
    for(auto it = $4->brothers.begin(); it != $4->brothers.end(); it++){
      ptr->type += ",";
      ptr->type += (*it)->type;
    }
  }
  ptr->type += ")";
  add_sym(ptr->name, ptr->type);
  ptr->addSon($4);
  if(!$4->brothers.empty()){
    ptr->addSons($4);
  }
  delete $1;
  delete $2;
  $$ = ptr;
}
;

FuncFParams: FuncFParam {
  if($1->kind != "ELLIPSIS"){
    $$ = $1;
  }
}
| FuncFParams T_COMMA FuncFParam {
  if($3->kind != "ELLIPSIS"){
    $1->addBrother($3);
  }
  $$ = $1;
}
;

FuncFParam: BType T_IDENTIFIER {
  auto ptr = new Tree("ParmVarDecl", $2->name, "", $1->type);
  add_sym(ptr->name, ptr->type);
  delete $1;
  delete $2;
  $$ = ptr;
}
| FuncFParam T_L_SQUARE T_R_SQUARE {

}
| FuncFParam T_L_SQUARE Exp T_R_SQUARE {

}
| T_ELLIPSIS {
  auto ptr = new Tree("ELLIPSIS");
  $$ = ptr;
}
;

Block: T_L_BRACE BlockItem T_R_BRACE {
  auto ptr = new Tree("CompoundStmt", "", "", "");
  ptr->addSon($2);
  if(!$2->brothers.empty()){
    ptr->addSons($2);
  }
  $$ = ptr;
}
| T_L_BRACE T_R_BRACE {
  auto ptr = new Tree("CompoundStmt", "", "", "");
  $$ = ptr;
}
;

BlockItem: Stmt {

} 
| BlockItem Stmt {
 $1->addBrother($2);
 $$ = $1;
}
| Block {

}
| BlockItem Block {
 $1->addBrother($2);
 $$ = $1;
}
;

Stmt: T_RETURN Exp T_SEMI {
  auto ptr = new Tree("ReturnStmt", "", "", $2->type);
  ptr->addSon($2);
  $$ = ptr;
}
| T_SEMI {
  $$ = new Tree("NullStmt", "", "", "");
}
| T_RETURN T_SEMI {
  auto ptr = new Tree("ReturnStmt", "", "", "void");
  $$ = ptr;
}
| VarDecl {
  auto ptr = new Tree("DeclStmt", "", "", $1->type);
  ptr->addSon($1);
  if(!$1->brothers.empty()){
    ptr->addSons($1);
  }
  $$ = ptr;
}
| LVal T_EQUAL Exp T_SEMI {
  auto ptr = new Tree("BinaryOperator", "=");
  ptr->type = get_sym_type($1->name);
  if(ptr->type != $3->type){
    auto imp = new Tree("ImplicitCastExpr", "", "", ptr->type);
    imp->addSon($3);
    ptr->addSon($1);
    ptr->addSon(imp);
  }
  else{
    ptr->addSon($1);
    ptr->addSon($3);
  }
  $$ = ptr;
}
| Exp T_SEMI{

}
| T_IF T_L_PAREN Cond T_R_PAREN Stmt %prec IF_THEN{
  auto ptr = new Tree("IfStmt", "", "", $3->type);
  ptr->addSon($3);
  ptr->addSon($5);
  $$ = ptr;
}
| T_IF T_L_PAREN Cond T_R_PAREN Stmt T_ELSE Stmt{
  auto ptr = new Tree("IfStmt", "", "", $3->type);
  ptr->addSon($3);
  ptr->addSon($5);
  ptr->addSon($7);
  $$ = ptr;
}
| T_IF T_L_PAREN Cond T_R_PAREN Block %prec IF_THEN{
  auto ptr = new Tree("IfStmt", "", "", $3->type);
  ptr->addSon($3);
  ptr->addSon($5);
  $$ = ptr;
}
| T_IF T_L_PAREN Cond T_R_PAREN Block T_ELSE Stmt{
  auto ptr = new Tree("IfStmt", "", "", $3->type);
  ptr->addSon($3);
  ptr->addSon($5);
  ptr->addSon($7);
  $$ = ptr;
}
| T_IF T_L_PAREN Cond T_R_PAREN Stmt T_ELSE Block{
  auto ptr = new Tree("IfStmt", "", "", $3->type);
  ptr->addSon($3);
  ptr->addSon($5);
  ptr->addSon($7);
  $$ = ptr;
}
| T_IF T_L_PAREN Cond T_R_PAREN Block T_ELSE Block{
  auto ptr = new Tree("IfStmt", "", "", $3->type);
  ptr->addSon($3);
  ptr->addSon($5);
  ptr->addSon($7);
  $$ = ptr;
}
| T_WHILE T_L_PAREN Cond T_R_PAREN Stmt {
  auto ptr = new Tree("WhileStmt", "", "", $3->type);
  ptr->addSon($3);
  ptr->addSon($5);
  $$ = ptr;
}
| T_WHILE T_L_PAREN Cond T_R_PAREN Block {
  auto ptr = new Tree("WhileStmt", "", "", $3->type);
  ptr->addSon($3);
  ptr->addSon($5);
  $$ = ptr;
}
| T_CONTINUE T_SEMI {
  auto ptr = new Tree("ContinueStmt", "", "", "");
  $$ = ptr;
}
| T_BREAK T_SEMI {
  auto ptr = new Tree("BreakStmt", "", "", "");
  $$ = ptr;
}
| T_DO Stmt T_WHILE T_L_PAREN Cond T_R_PAREN T_SEMI {
  auto ptr = new Tree("DoStmt", "", "", "");
  ptr->addSon($2);
  ptr->addSon($5);
  $$ = ptr;
}
| T_DO Block T_WHILE T_L_PAREN Cond T_R_PAREN T_SEMI {
  auto ptr = new Tree("DoStmt", "", "", "");
  ptr->addSon($2);
  ptr->addSon($5);
  $$ = ptr;
}
;

LVal: T_IDENTIFIER {
  auto ptr = new Tree("DeclRefExpr", $1->name);
  // 查找符号表
  ptr->type = get_sym_type(ptr->name);
  delete $1;
  $$ = ptr;
}
| LVal T_L_SQUARE Exp T_R_SQUARE {
  auto ptr = new Tree("ArraySubscriptExpr", $1->name);
  auto son = new Tree("ImplicitCastExpr", "ArrayToPointerDecay", "", "");
  ptr->type = get_sym_type(ptr->name);
  son->addSon($1);
  ptr->addSon(son);
  ptr->addSon($3);
  $$ = ptr;
}
| T_L_PAREN LVal T_R_PAREN {
  auto ptr = new Tree("ParenExpr", "", "", $2->type);
  ptr->addSon($2);
  $$ = ptr;
}
;

Exp: AddExp {

}
;

Cond: LOrExp {

};

AddExp: MulExp {

}
| AddExp T_PLUS MulExp {
  auto ptr = new Tree("BinaryOperator", "", "+");
  BinaryOpImplicitCast(ptr, $1, $3);
  $$ = ptr;
}
| AddExp T_MINUS MulExp {
  auto ptr = new Tree("BinaryOperator", "", "-");
  BinaryOpImplicitCast(ptr, $1, $3);
  $$ = ptr;
}
;

RelExp: AddExp {

}
| RelExp T_GREATER AddExp {
  auto ptr = new Tree("BinaryOperator", "", ">");
  BinaryOpImplicitCast(ptr, $1, $3);
  $$ = ptr;
}
| RelExp T_GREATEREQUAL AddExp {
  auto ptr = new Tree("BinaryOperator", "", ">=");
  BinaryOpImplicitCast(ptr, $1, $3);
  $$ = ptr;
}
| RelExp T_LESS AddExp {
  auto ptr = new Tree("BinaryOperator", "", "<");
  BinaryOpImplicitCast(ptr, $1, $3);
  $$ = ptr;
}
| RelExp T_LESSEQUAL AddExp {
  auto ptr = new Tree("BinaryOperator", "", "<=");
  BinaryOpImplicitCast(ptr, $1, $3);
  $$ = ptr;
}
;

EqExp: RelExp {

}
| EqExp T_EQUALEQUAL RelExp {
  auto ptr = new Tree("BinaryOperator", "", "==");
  BinaryOpImplicitCast(ptr, $1, $3);
  $$ = ptr;
}
| EqExp T_EXCLAIMEQUAL RelExp {
  auto ptr = new Tree("BinaryOperator", "", "!=");
  BinaryOpImplicitCast(ptr, $1, $3);
  $$ = ptr;
}
;

LAndExp: EqExp {

}
| LAndExp T_AMPAMP EqExp {
  auto ptr = new Tree("BinaryOperator", "", "&&", "");
  ptr->addSon($1);
  ptr->addSon($3);
  $$ = ptr;
}
;

LOrExp: LAndExp {

}
| LOrExp T_PIPEPIPE LAndExp {
  auto ptr = new Tree("BinaryOperator", "", "||", "");
  ptr->addSon($1);
  ptr->addSon($3);
  $$ = ptr;
}
;

MulExp: UnaryExp {

}
| MulExp T_STAR UnaryExp {
  auto ptr = new Tree("BinaryOperator", "", "*");
  BinaryOpImplicitCast(ptr, $1, $3);
  $$ = ptr;
}
| MulExp T_SLASH UnaryExp {
  auto ptr = new Tree("BinaryOperator", "", "/");
  BinaryOpImplicitCast(ptr, $1, $3);
  $$ = ptr;
}
| MulExp T_PERCENT UnaryExp {
  auto ptr = new Tree("BinaryOperator", "", "%");
  BinaryOpImplicitCast(ptr, $1, $3);
  $$ = ptr;
}
;

UnaryExp: PrimaryExp {

}
| UnaryOp UnaryExp {
  $1->type = $2->type;
  $1->addSon($2);
  $$ = $1;
} 
| T_IDENTIFIER T_L_PAREN T_R_PAREN {
  std::string raw_type = get_sym_type($1->name);
  if(raw_type == "int"){
    // 处理递归定义
    raw_type += " ()";
  }
  std::string ret_type = "int";
  if(SymTable.get($1->name) != nullptr){
    ret_type = SymTable.getString($1->name)->split(' ').first.str();
  }
  auto ptr = new Tree("CallExpr", "", "", ret_type);
  auto son = new Tree("ImplicitCastExpr", "", "", "func(*)");
  auto grandson = new Tree("DeclRefExpr", $1->name, "", raw_type);
  son->addSon(grandson);
  ptr->addSon(son);
  delete $1;
  $$ = ptr;
}
| T_IDENTIFIER T_L_PAREN FuncRParams T_R_PAREN {
// 不能单独处理
  // if($3->type == "char"){
  //   // 处理字符串
  //   auto ptr = new Tree("CallExpr", "", "", "long long");
  //   auto func = new Tree("ImplicitCastExpr", "", "", "func(*)");
  //   auto func_son = new Tree("DeclRefExpr", $1->name, "", "");
  //   func->addSon(func_son);
  //   ptr->addSon(func);
  //   auto param_imp = new Tree("ImplicitCastExpr", "", "", "");
  //   auto param_imp_son = new Tree("ImplicitCastExpr", "", "", "");
  //   param_imp_son->addSon($3);
  //   param_imp->addSon(param_imp_son);
  //   ptr->addSon(param_imp);
  //   delete $1;
  //   $$ = ptr;
  // }
  // else{
    std::string raw_type = get_sym_type($1->name);
    if(raw_type == "int"){
      // 处理递归定义
      raw_type += " (unknown)";
    }
    std::string ret_type = "int";
    std::vector<std::string> params_type;  // 默认值该如何处理?
    if(SymTable.get($1->name) != nullptr){
      ret_type = raw_type.substr(0, raw_type.find(' '));
      // 只有一个参数
      if($3->brothers.empty()){
        params_type.push_back(raw_type.substr(raw_type.find('(')+1, raw_type.find(')')-raw_type.find('(')-1));
      }
      // 多个参数
      else{
        int l = raw_type.find('(')+1, r = raw_type.find(',');
        // first one
        params_type.push_back(raw_type.substr(l, r-l));
        for(auto it = $3->brothers.begin(); it != $3->brothers.end()-1; it++){
          l = r+1, r = raw_type.find(',', l);
          params_type.push_back(raw_type.substr(l, r-l));
          // std::cout<<raw_type.substr(l, r-l)<<std::endl;
        }
        // last one
        l = r+1, r = raw_type.find(')', l);
        params_type.push_back(raw_type.substr(l, r-l));
        // std::cout<<raw_type.substr(l, r-l)<<std::endl;
      }
    }
    auto ptr = new Tree("CallExpr", "", "", ret_type);
    auto func = new Tree("ImplicitCastExpr", "", "", "func(*)");
    auto func_son = new Tree("DeclRefExpr", $1->name, "", raw_type);
    func->addSon(func_son);
    ptr->addSon(func);

    if(raw_type == "int (unknown)"){
    // 递归函数不做隐式转换处理
      ptr->addSon($3);
      if(!$3->brothers.empty()){
        ptr->addSons($3);
      }
    }
    else{
      // 判断是否有隐式转换
      // 第一个参数
      if($3->type != params_type[0]){
          auto first_param = new Tree("ImplicitCastExpr", "", "", params_type[0]);
          first_param->addSon($3);
          ptr->addSon(first_param);
        }
      else{
        ptr->addSon($3);
      }
      // 其他参数
      if(!$3->brothers.empty()){
        int i = 1;
        for(auto it = $3->brothers.begin(); it != $3->brothers.end(); it++){
          if((*it)->type != params_type[i]){
              auto other_param = new Tree("ImplicitCastExpr", "", "", params_type[i]);
              other_param->addSon(std::move(*it));
              ptr->addSon(std::move(other_param));
            }
          else{
            ptr->addSon(std::move(*it));
          }
          i++;
        }
      }
    }

    delete $1;
    $$ = ptr;
  // }
}
;


FuncRParams: Exp {
  
}
| FuncRParams T_COMMA Exp {
  $1->addBrother($3);
  $$ = $1;
}
;

PrimaryExp: T_L_PAREN Exp T_R_PAREN {
  // // 特判ImplicitCastExpr
  // if($2->kind == "ImplicitCastExpr"){
  //   auto ptr = new Tree("ImplicitCastExpr");
  //   $2->kind = "ParenExpr";
  //   ptr->addSon($2);
  //   $$ = ptr;
  // }
  // else{
    auto ptr = new Tree("ParenExpr", "", "", $2->type);
    ptr->addSon($2);
    $$ = ptr;
  // }
}
| T_NUMERIC_CONSTANT {
  
}
| LVal %prec PrimLVal {
// | LVal {
  auto ptr = new Tree("ImplicitCastExpr", "LValueToRValue", "", $1->type);
  ptr->addSon($1);
  $$ = ptr;
}
| T_STRING {

}
;

UnaryOp: T_PLUS {
  auto ptr = new Tree("UnaryOperator", "", "+");
  // delete $1;
  $$ = ptr;
}
| T_MINUS {
  auto ptr = new Tree("UnaryOperator", "", "-");
  // delete $1;
  $$ = ptr;
}
| T_EXCLAIM {
  auto ptr = new Tree("UnaryOperator", "", "!");
  $$ = ptr;
}
;
// TO-DO：你需要在这里实现文法和树，通过测例
%%
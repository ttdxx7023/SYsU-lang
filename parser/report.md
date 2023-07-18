1. 进制
- 将`s\t`从`auto s = tk.substr(b, e - b).str(), t = tk.substr(0, tk.find(" ")).str();`变为`auto s = tk.substr(b, e - b), t = tk.substr(0, tk.find(" "));`
- 增加
```cpp
    long num;
    s.getAsInteger(0, num);
    yylval = new Tree("IntegerLiteral", "", std::to_string(num));
```
- 修改`yylval = new Tree("id", s);`为`yylval = new Tree("id", s.str());`
2. if-else
```
%right IF_THEN T_ELSE
Stmt: T_IF T_L_PAREN Cond T_R_PAREN Stmt  %prec IF_THEN
| T_IF T_L_PAREN Cond T_R_PAREN Stmt T_ELSE Stmt
```
3. block中含有block
改写block item
```
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
```
不能改写stmt增加block, 会导致许多rs、rr冲突, 可以利用命令`bison /workspace/SYsU-lang/parser/parser.y -Wcounterexamples`查看
4. 多维数组
初始版本: 不能过`int a[1][1]={{}};`, **存疑**
```cpp
InitVal: Exp {

}
| InitValList {
    auto ptr = new Tree("InitListExpr");
    ptr->addSon($2);
    if(!$2->brothers.empty()){
        ptr->addSons($2);
    }
    $$ = ptr;
}
;

InitValList: Exp {
    // int a[1] = {1};
}
| InitValList T_COMMA Exp {
    // int a[2] = {1, 2};
    $1->addBrother($3);
    $$ = $1;
}
| T_L_BRACE InitValList T_R_BRACE {
    // int a[2][2] = {{1,2}};
}
| InitValList T_COMMA T_L_BRACE InitValList T_R_BRACE {
    // int a[2][2] = {{1,2}, {3,4}};
    $1->addBrother($4);
    $$ = $1;
}
| {
    // int a[1] = {};
} 
;
```
5. 处理LVal的ImplicitCastExpr与()的父子关系
对于
```c
(a);
```
clang 输出:
```
|     `-ImplicitCastExpr 0x24761b0 <col:10, col:12> 'int' <LValueToRValue>
|       `-ParenExpr 0x2476190 <col:10, col:12> 'int' lvalue
|         `-DeclRefExpr 0x2476170 <col:11> 'int' lvalue Var 0x2475ea8 'a' 'int'
```
对于
```c
(1);
```
clang 输出:
```
|-ParenExpr 0x21c4000 <line:17:5, col:7> 'int'
| `-IntegerLiteral 0x21c3fe0 <col:6> 'int' 1
```
解决方法:
1. 特判
```cpp
PrimaryExp: T_L_PAREN Exp T_R_PAREN {
    // 特判ImplicitCastExpr
    if($2->kind == "ImplicitCastExpr"){
      auto ptr = new Tree("ImplicitCastExpr");
      $2->kind = "ParenExpr";
      ptr->addSon($2);
      $$ = ptr;
    }
    else{
        auto ptr = new Tree("ParenExpr");
        ptr->addSon($2);
        $$ = ptr;
    }
}
```
2. LVal增加RHS: `(` LVal `)`
```cpp
LVal: 
| T_L_PAREN LVal T_R_PAREN {
    auto ptr = new Tree("ParenExpr");
    ptr->addSon($2);
    $$ = ptr;
}
;
```
会带来s-r冲突:
```bash
/workspace/SYsU-lang/parser/parser.y: warning: 1 shift/reduce conflict [-Wconflicts-sr]
/workspace/SYsU-lang/parser/parser.y: warning: shift/reduce conflict on token T_R_PAREN [-Wcounterexamples]
    Example: T_L_PAREN LVal . T_R_PAREN
    Shift derivation
        PrimaryExp
        `-> LVal
            `-> T_L_PAREN LVal . T_R_PAREN
    Reduce derivation   
        PrimaryExp
        `-> T_L_PAREN Exp                        T_R_PAREN
                    `-> AddExp
                        `-> MulExp
                            `-> UnaryExp
                                `-> PrimaryExp
                                    `-> LVal .
```
指定优先级LVal先与右括号`)`结合:
```cpp
%right PrimLVal T_R_PAREN
PrimaryExp: 
    | LVal %prec PrimLVal
```
6. 浮点精度
https://github.com/arcsysu/SYsU-lang/discussions/77
6. 隐式转换
```cpp
const int k0 = -2147483648;
```
7. 默认为int
8. 函数返回值的隐式转换
思路：`return` 被包含在 `{}` 即`CompoundStmt`中, 检查每个`CompoundStmt` 是否包含`return`语句
9. 再看看前面有的寄掉了
/workspace/SYsU-lang/tester/function_test2020/50_recursion_test1.sysu.c
符号表问题，导致没有第一个参数类型params_type[0]不存在而出错
10. 
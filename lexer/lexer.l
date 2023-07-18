%{
#include <cctype>
#include <cstdio>
#include <string>
#include <stdio.h>
#define YYEOF 0
int yylex();
// std::string is_newline();
// std::string is_newspace();
std::string genflag();
int main() {
  do {
  } while (yylex() != YYEOF);
}
std::string yyloc = "<stdin>";
int yyrow = 1, yycolumn = 1, yycolpre = 1;
int yynewline = 1, yynewspace = 0;
// std::string is_newline(){
//   std::string s = "";
//   if(yynewline)
//     s = "[StartOfLine]";
//     yynewline = 0;
//   return s;
// }
// std::string is_newspace(){
//   std::string s = "";
//   if(yynewspace)
//     s = "[LeadingSpace]";
//     yynewspace = 0;
//   return s;  
// }
std::string genflag(){
  std::string flag;
  if(yynewline && yynewspace){
    flag = " [StartOfLine] [LeadingSpace]";
  }
  else if(yynewline){
    flag = " [StartOfLine]";
  }
  else if(yynewspace){
    flag = " [LeadingSpace]";
  }
  else{
    flag = "";
  }
  yynewline = yynewspace = 0;
  return flag;
}
#define YY_USER_ACTION                                                         \
  do {                                                                         \
    yycolumn += yyleng;                                                        \
  } while (0);
%}
%option noyywrap
%%
#.* {
  std::string s(yytext);
  auto l = s.find("\""), r = s.rfind("\"");
  yyloc = s.substr(l + 1, r - l - 1);
  for (int i = 0; i < s.size(); ++i)
    if (std::isdigit(s[i])) {
      for (yyrow = 0; i < s.size() && std::isdigit(s[i]); ++i)
        yyrow = yyrow * 10 + s[i] - '0';
      --yyrow;
      break;
    }
}
\n {
  ++yyrow;
  yycolpre = yycolumn;
  yycolumn = 1;
  yynewline = 1;
}
[\f\r\t\v ] {
  yynewspace = 1;
}

const {
  std::fprintf(yyout, "const '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), 
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;    
}
, {
  std::fprintf(yyout, "comma '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;    
}
; {
  std::fprintf(yyout, "semi '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
: {
  std::fprintf(yyout, "colon '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
auto {
  std::fprintf(yyout, "auto '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;  
}
int {
  std::fprintf(yyout, "int '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
float {
  std::fprintf(yyout, "float '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
double {
  std::fprintf(yyout, "double '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
long {
  std::fprintf(yyout, "long '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
enum {
  std::fprintf(yyout, "enum '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
register {
  std::fprintf(yyout, "register '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
short {
  std::fprintf(yyout, "short '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
signed {
  std::fprintf(yyout, "signed '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
unsigned {
  std::fprintf(yyout, "unsigned '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
static {
  std::fprintf(yyout, "static '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;  
}
struct {
  std::fprintf(yyout, "struct '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
union {
  std::fprintf(yyout, "union '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
typedef {
  std::fprintf(yyout, "typedef '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
void {
  std::fprintf(yyout, "void '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
char {
  std::fprintf(yyout, "char '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;    
}
\[ {
  std::fprintf(yyout, "l_square '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;    
}
\] {
  std::fprintf(yyout, "r_square '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;    
}
= {
  std::fprintf(yyout, "equal '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;    
}
\{ {
  std::fprintf(yyout, "l_brace '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\} {
  std::fprintf(yyout, "r_brace '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\( {
  std::fprintf(yyout, "l_paren '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\) {
  std::fprintf(yyout, "r_paren '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
if {
  std::fprintf(yyout, "if '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;      
}
else {
  std::fprintf(yyout, "else '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;      
}
while {
  std::fprintf(yyout, "while '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;  
}
do {
  std::fprintf(yyout, "do '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;    
}
break {
  std::fprintf(yyout, "break '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
continue {
  std::fprintf(yyout, "continue '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;    
}
return {
  std::fprintf(yyout, "return '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
switch {
  std::fprintf(yyout, "switch '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
case {
  std::fprintf(yyout, "case '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;  
}
default {
  std::fprintf(yyout, "default '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;  
}
for {
  std::fprintf(yyout, "for '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
goto {
  std::fprintf(yyout, "goto '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
sizeof {
  std::fprintf(yyout, "sizeof '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
extern {
  std::fprintf(yyout, "extern '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
[0-9]+|0[xX][0-9a-fA-F]+|0[0-7]+|0[bB][0-1]+|[1-9] {
  std::fprintf(yyout, "numeric_constant '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
["."]*[0-9]+[eE][-+]*[0-9]+["."]* { // e-计数法
  std::fprintf(yyout, "numeric_constant '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
[0-9]*"."[0-9]* { // 小数点计数法
  std::fprintf(yyout, "numeric_constant '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
0[xX][0-9a-fA-F]*"."[0-9a-fA-F]*[pP][-+]*[0-9a-fA-F]* { // p-计数法
  std::fprintf(yyout, "numeric_constant '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
"..." { // 省略号
  std::fprintf(yyout, "ellipsis '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\+ {
  std::fprintf(yyout, "plus '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\+\+ {
  std::fprintf(yyout, "plusplus '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\+\= {
  std::fprintf(yyout, "plusequal '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;  
}
\- {
  std::fprintf(yyout, "minus '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\-\- {
  std::fprintf(yyout, "minusminus '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\-\= {
  std::fprintf(yyout, "minusequal '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\! {
  std::fprintf(yyout, "exclaim '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\* {
  std::fprintf(yyout, "star '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\*\= {
  std::fprintf(yyout, "starequal '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\/ {
  std::fprintf(yyout, "slash '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\/\= {
  std::fprintf(yyout, "slashequal '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\% {
  std::fprintf(yyout, "percent '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\%\= {
  std::fprintf(yyout, "percentequal '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\< {
  std::fprintf(yyout, "less '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\> {
  std::fprintf(yyout, "greater '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\<\= {
  std::fprintf(yyout, "lessequal '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\>\= {
  std::fprintf(yyout, "greaterequal '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\=\= {
  std::fprintf(yyout, "equalequal '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\!\= {
  std::fprintf(yyout, "exclaimequal '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\&\& {
  std::fprintf(yyout, "ampamp '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\|\| {
  std::fprintf(yyout, "pipepipe '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
<<EOF>> {
  std::fprintf(yyout, "eof ''\t\tLoc=<%s:%d:%d>\n", yyloc.c_str(), yyrow - 1,
               yycolpre - yyleng);
  return YYEOF;
}
[a-zA-Z_][a-zA-Z_0-9]* { // identifier
  std::fprintf(yyout, "identifier '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
\"([^\"\\]|\\.)*\" { // 中间没有" | 中间有"
  // \"[^"]\"
  // \"(\\.|[^"\\])*\"
  std::fprintf(yyout, "string_literal '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(),
               yyloc.c_str(), yyrow, yycolumn - yyleng);
  return ~YYEOF;    
}
\/\/.* {
  // 单行注释
}

. {
  std::fprintf(yyout, "unknown '%s'\t%s\tLoc=<%s:%d:%d>\n", yytext, genflag().c_str(), yyloc.c_str(),
               yyrow, yycolumn - yyleng);
  return ~YYEOF;
}
%%
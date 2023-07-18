#include "JsonToASG.hpp"
#include "EmitIR.hpp"
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

// #define DEBUG 0
// void debug_out(std::string str){
//     if(DEBUG) llvm::errs()<<str<<'\n';
// }

llvm::LLVMContext context;
llvm::Module module("-", context);

int main(){
    auto llvmin = llvm::MemoryBuffer::getFileOrSTDIN("-");
    auto json = llvm::json::parse(llvmin.get()->getBuffer());
    // json -> asg
    // llvm::errs()<<"--------json -> asg--------\n";
    // debug_out("--------json -> asg--------\n");
    JsonToASG json_to_asg;
    TranslationUnitDecl* root = json_to_asg.TranslationUnitDeclToASG(json->getAsObject(), &context);
    // asg -> ir
    // llvm::errs()<<"--------asg -> ir--------\n";
    // debug_out("--------asg -> ir--------\n");
    EmitIR emit_ir;
    emit_ir.TranslationUnitDeclToIR(root);
    // print
    module.print(llvm::outs(), nullptr);
    return 0;
}
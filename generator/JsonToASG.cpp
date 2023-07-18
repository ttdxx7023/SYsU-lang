#include "JsonToASG.hpp"

// --------TranslationUnitDecl--------
TranslationUnitDecl* JsonToASG::TranslationUnitDeclToASG(llvm::json::Object* object, llvm::LLVMContext *context){
    // debug_out("TranslationUnitDeclToASG");
    if(object == nullptr) return nullptr;
    if (auto kind = object->get("kind")->getAsString()) {
        assert(*kind == "TranslationUnitDecl");
    } 
    else {
        assert(0);
    }
    
    this->context = context;
    symbol_table.clear();
    TranslationUnitDecl* root = new TranslationUnitDecl();

    auto inner = object->getArray("inner");
    for (auto& it: *inner)
    {
        auto it_object = it.getAsObject();
        if (it_object != nullptr)
        {
            std::string kind = it_object->getString("kind").getValue().str();
            if (kind == "VarDecl") 
            {
                // 全局变量
                root->global_var_decls.emplace_back(VarDeclToASG(it_object));
            }
            else if (kind == "FunctionDecl")
            {
                // 函数
                // 存在处理内置函数返回空指针的情况
                auto function_decl = FunctionDeclToASG(it_object);
                if (function_decl != nullptr)
                {
                    root->function_decls.emplace_back(function_decl);
                }
            }
        }
    }
    
    return root;
}

// --------Type--------
Type JsonToASG::getType(llvm::json::Object* object){
    std::string type_str = object->getObject("type")->getString("qualType").getValue().str();

    // is_const
    bool is_const = false;
    auto space_pos = type_str.find_first_of(' ');
    std::string rem_str = type_str;
    if (space_pos != std::string::npos)
    {   // "const "
        if (type_str.substr(0, space_pos) == "const")
        {
            is_const = true;
            rem_str = type_str.substr(space_pos+1);
        }
    }

    // unsigned 
    bool is_unsigned = false;
    space_pos = rem_str.find_first_of(' ');
    if (space_pos != std::string::npos)
    {
        if (rem_str.substr(0, space_pos) == "unsigned")
        {
            is_unsigned = true;
            rem_str = rem_str.substr(space_pos+1);
        }
    }

    // 基本类型
    std::string basic_type_str = rem_str;
    space_pos = rem_str.find_first_of(' ');
    if (space_pos != std::string::npos)
    {
        basic_type_str = rem_str.substr(0, space_pos);
    }
    llvm::Type* type = getBasicType(basic_type_str);

    // 指针类型
    int ptr_pos = type_str.find_first_of('*');
    if (ptr_pos != std::string::npos && type_str[ptr_pos-1] != '(')
    {
        type = getPtrType(type, type_str);
    }  
    
    // 数组类型
    if(type_str.find('[') != std::string::npos){
        type = getArrayType(type, type_str);
    }

    // int a[][5]: int (*)[5]) 
    if (type_str.find("(*)") != std::string::npos) {
        type = llvm::PointerType::getUnqual(type);
        // if(type->getTypeID() == llvm::Type::PointerTyID) debug_out(type_str);
    }

    return Type(type, is_const);
}

llvm::Type* JsonToASG::getBasicType(const std::string& basic_type_str){
    if(basic_type_str == "int") return llvm::Type::getInt32Ty(*context);
    else if(basic_type_str == "float") return llvm::Type::getFloatTy(*context);
    else if(basic_type_str == "double") return llvm::Type::getDoubleTy(*context);
    else if(basic_type_str == "long" || basic_type_str == "long long") return llvm::Type::getInt64Ty(*context);
    else if(basic_type_str == "char") return llvm::Type::getInt8Ty(*context);
    else if(basic_type_str == "void") return llvm::Type::getVoidTy(*context);

    return nullptr;
}

llvm::Type* JsonToASG::getPtrType(llvm::Type* basic_type, const std::string& type_str){
    // 计算指针数量
    std::string ptr_str = type_str;
    int ptr_num = 0;
    int ptr_pos = ptr_str.find('*');
    while (ptr_pos != std::string::npos && ptr_str[ptr_pos-1] != '(')
    {
        ptr_num++;
        ptr_str = ptr_str.substr(ptr_pos+1);
        ptr_pos = ptr_str.find('*');
    }

    // 构造指针类型
    llvm::Type* type = basic_type;
    // if(ptr_num) debug_out("suc*");
    for (int i = 0; i < ptr_num; i++)
    {
        type = llvm::PointerType::getUnqual(type);
    }
    
    return type;
}

llvm::Type* JsonToASG::getArrayType(llvm::Type* element_type, const std::string& type_str){
    std::string arr_str = type_str;
    int left_pos = arr_str.rfind('[');
    int right_pos = arr_str.rfind(']');
    llvm::Type* type = element_type;

    while (left_pos != std::string::npos)
    {
        unsigned int size = std::stoi(arr_str.substr(left_pos+1, right_pos-left_pos-1));
        type = llvm::ArrayType::get(type, size);
        arr_str = arr_str.substr(0, left_pos);
        left_pos = arr_str.rfind('[');
        right_pos = arr_str.rfind(']');
    }
    
    return type;
}

llvm::FunctionType* JsonToASG::getFunctionType(llvm::json::Object* object){
    std::string type_str = object->getObject("type")->getString("qualType").getValue().str();

    // 返回值
    std::string ret_type_str = type_str.substr(0, type_str.find_first_of('(')-1);
    llvm::Type* ret_type = getStrType(ret_type_str);

    // 参数类型
    std::vector<llvm::Type*> parms_type;
    if (type_str.find_first_of('(')+1 != type_str.find_last_of(')'))
    {
        std::string parms_type_str = type_str.substr(type_str.find_first_of('(')+1);
        int comma_pos = parms_type_str.find(',');
        while (comma_pos != std::string::npos)
        {
            std::string parm_type_str = parms_type_str.substr(0, comma_pos);
            llvm::Type* parm_type = getStrType(parm_type_str);
            parms_type.emplace_back(parm_type);
            parms_type_str = parms_type_str.substr(comma_pos+2);
            comma_pos = parms_type_str.find(',');
        }
        // 只有一个参数 或 最后一个参数
        std::string parm_type_str = parms_type_str.substr(0, parms_type_str.find_last_of(')'));
        if (parm_type_str != std::string("..."))
        {
            llvm::Type* parm_type = getStrType(parm_type_str);
            parms_type.emplace_back(parm_type);
        }
    }
    // 参数类型
    llvm::ArrayRef<llvm::Type*> parms_type_(parms_type);
    // 可变参数标志
    bool is_variable_parm = (type_str.find("...") != std::string::npos);

    return llvm::FunctionType::get(ret_type, parms_type_, is_variable_parm);
}

llvm::Type* JsonToASG::getStrType(const std::string& type_str){
    // is_const
    bool is_const = false;
    auto space_pos = type_str.find_first_of(' ');
    std::string rem_str = type_str;
    if (space_pos != std::string::npos)
    {
        if (type_str.substr(0, space_pos) == "const")
        {
            is_const = true;
            rem_str = type_str.substr(space_pos+1);
        }
    }

    // unsigned 
    bool is_unsigned = false;
    space_pos = rem_str.find_first_of(' ');
    if (space_pos != std::string::npos)
    {
        if (rem_str.substr(0, space_pos) == "unsigned")
        {
            is_unsigned = true;
            rem_str = rem_str.substr(space_pos+1);
        }
    }

    // 基本类型
    std::string basic_type_str = rem_str;
    space_pos = rem_str.find_first_of(' ');
    if (space_pos != std::string::npos)
    {
        basic_type_str = rem_str.substr(0, space_pos);
    }
    llvm::Type* type = getBasicType(basic_type_str);

    // 指针类型
    int ptr_pos = type_str.find_first_of('*');
    if (ptr_pos != std::string::npos && type_str[ptr_pos-1] != '(')
    {
        // debug_out("has*");
        type = getPtrType(type, type_str);
    }  
    
    // 数组类型
    if(type_str.find('[') != std::string::npos){
        type = getArrayType(type, type_str);
    }

    // int a[][5]: int (*)[5]) 
    if (type_str.find("(*)") != std::string::npos) {
        type = llvm::PointerType::getUnqual(type);
    }

    return type;
}

// --------基类--------
Obj* JsonToASG::ObjToASG(llvm::json::Object* object){
    // debug_out("ObjToASG");
    if(object == nullptr) return nullptr;

    std::string kind = object->getString("kind").getValue().str();
    // 注意DeclStmt的情况, 所以Stmt先于Decl
    if(kind.find("Stmt") != std::string::npos) return StmtToASG(object);
    else if(kind.find("Decl") != std::string::npos) return DeclToASG(object);
    else if(kind.empty()) return nullptr;

    // 注意有的表达式kind不包含"Expr", 例如二元运算表达式kind为"BinaryOperator"
    return ExprToASG(object);
}

Decl* JsonToASG::DeclToASG(llvm::json::Object* object){
    // debug_out("DeclToASG");
    if(object == nullptr) return nullptr;

    std::string kind = object->getString("kind").getValue().str();
    // debug_out(kind);
    if(kind == "VarDecl") return VarDeclToASG(object);
    else if(kind == "ParmVarDecl") return ParmVarDeclToASG(object);
    else if(kind == "FunctionDecl") return FunctionDeclToASG(object);

    return nullptr;
}

Expr* JsonToASG::ExprToASG(llvm::json::Object* object){
    // debug_out("ExprToASG");
    if(object == nullptr) return nullptr;
    
    std::string kind = object->getString("kind").getValue().str();
    if(kind == "IntegerLiteral") return IntegerLiteralToASG(object);
    else if(kind == "FloatingLiteral") return FloatingLiteralToASG(object);
    else if(kind == "CharacterLiteral") return CharacterLiteralToASG(object);
    else if(kind == "StringLiteral") return StringLiteralToASG(object);
    else if(kind == "UnaryOperator") return UnaryExprToASG(object);
    else if(kind == "BinaryOperator") return BinaryExprToASG(object);
    else if(kind == "ParenExpr") return ParenExprToASG(object);
    else if(kind == "DeclRefExpr") return DeclRefExprToASG(object);
    else if(kind == "InitListExpr") return InitListExprToASG(object);
    else if(kind == "ArraySubscriptExpr") return ArraySubscriptExprToASG(object);
    else if(kind == "ImplicitCastExpr") return ImplicitCastExprToASG(object);
    else if(kind == "CallExpr") return CallExprToASG(object);

    return nullptr;
}

Stmt* JsonToASG::StmtToASG(llvm::json::Object* object){
    // debug_out("StmtToASG");
    if(object == nullptr) return nullptr;
    
    std::string kind = object->getString("kind").getValue().str();
    if(kind == "CompoundStmt") return CompoundStmtToASG(object);
    else if(kind == "NullStmt") return NullStmtToASG(object);
    else if(kind == "ReturnStmt") return ReturnStmtToASG(object);
    else if(kind == "DeclStmt") return DeclStmtToASG(object);
    else if(kind == "IfStmt") return IfStmtToASG(object);
    else if(kind == "WhileStmt") return WhileStmtToASG(object);
    else if(kind == "DoStmt") return DoStmtToASG(object);
    else if(kind == "BreakStmt") return BreakStmtToASG(object);
    else if(kind == "ContinueStmt") return ContinueStmtToASG(object);

    return nullptr;
}

// --------Decl--------
std::string JsonToASG::getName(llvm::json::Object* object){
    return object->getString("name").getValue().str().substr(0, 128);
}

void JsonToASG::registerID(llvm::json::Object* object, Decl* decl){
    int id = std::stoi(object->getString("id").getValue().str(), nullptr, 16);
    symbol_table.emplace(id, decl);
}

void JsonToASG::LogoutID(llvm::json::Object* object){
    int id = std::stoi(object->getString("id").getValue().str(), nullptr, 16);
    symbol_table.erase(id);
}

VarDecl* JsonToASG::VarDeclToASG(llvm::json::Object* object){
    // debug_out("VarDeclToASG");
    Type type = getType(object);
    std::string name = getName(object);
    VarDecl* ptr = new VarDecl(type, name);
    
    // 注册入符号表
    registerID(object, ptr);

    // init
    if (auto inner = object->getArray("inner"))
    {
        auto inner_object = inner->begin()->getAsObject();

        // 优化
        // 检测初始化是否包括函数调用
        if (inner_object->getString("kind").getValue() == "CallExpr")
        {
            ptr->is_promotable = false;
        }

        ptr->init_expr = ExprToASG(inner_object);
    }

    return ptr;
}

ParmVarDecl* JsonToASG::ParmVarDeclToASG(llvm::json::Object* object){
    // debug_out("ParmVarDeclToASG");
    if(object == nullptr) return nullptr;

    Type type = getType(object);
    // 函数参数定义json中没有key为name
    // std::string name = getName(object);
    ParmVarDecl* ptr = new ParmVarDecl(type, "");
    registerID(object, ptr);

    return ptr;
}

FunctionDecl* JsonToASG::FunctionDeclToASG(llvm::json::Object* object){
    // debug_out("FunctionDeclToASG");
    // 处理与库冲突的函数
    if(object->getString("storageClass").getValueOr("").str() == "extern") return nullptr;

    llvm::FunctionType* function_type = getFunctionType(object);
    std::string name = getName(object);
    FunctionDecl* ptr = new FunctionDecl(function_type, name, std::vector<ParmVarDecl*>());
    registerID(object, ptr); // 对于递归函数而言，需要在处理函数体之前就注册入符号表

    auto inner = object->getArray("inner");
    if(inner == nullptr) return ptr; // 内置函数
    
    for (auto& it: *inner)
    {
        auto it_object = it.getAsObject();
        std::string kind = it_object->getString("kind").getValue().str();
        if (kind == "ParmVarDecl")
        {
            ptr->parm_var_decls.emplace_back(ParmVarDeclToASG(it_object));
        }
        else if (kind == "CompoundStmt")
        {
            ptr->block = CompoundStmtToASG(it_object);
        }
    }

    return ptr;
}

// --------Expr--------
IntegerLiteral* JsonToASG::IntegerLiteralToASG(llvm::json::Object* object){
    // debug_out("IntegerLiteralToASG");
    Type type = getType(object);
    unsigned long long value = std::stoull(object->getString("value").getValue().str());
    return new IntegerLiteral(type, value);
}

FloatingLiteral* JsonToASG::FloatingLiteralToASG(llvm::json::Object* object){
    // debug_out("FloatingLiteralToASG");
    Type type = getType(object);
    double value = std::stod(object->getString("value").getValue().str());
    return new FloatingLiteral(type, value);
}

CharacterLiteral* JsonToASG::CharacterLiteralToASG(llvm::json::Object* object){
    // debug_out("CharacterLiteralToASG");
    Type type = getType(object);
    char value = std::stoi(object->getString("value").getValue().str());
    return new CharacterLiteral(type, value);
}

std::string replace_escape_c(std::string init, char matched_c){
    int size = init.size();
    if(size == 1) return init;
    
    char replace_c = matched_c;
    if(replace_c == 'n') replace_c = '\n';
    std::string res;
    int i = 1;
    while(i < size)
    {
        if(init[i-1] == '\\' && init[i] == matched_c){
            res += replace_c;
            i += 2;
        }
        else{
            res += init[i-1];
            i++;
        }
        if (i==size)
        {
            res += init[size-1];
        }
    }
    return res;
}
StringLiteral* JsonToASG::StringLiteralToASG(llvm::json::Object* object){
    // debug_out("StringLiteralToASG");
    Type type = getType(object);
    // debug_out("--------------------json value");
    // llvm::errs()<<object->getString("value").getValue()<<'\n';
    // debug_out("--------------------json value to string");
    std::string raw_str = object->getString("value").getValue().str();
    // debug_out(raw_str);
    // llvm::errs()<<raw_str.size()<<'\n';
    // const char r[9] = "\\\\\\\""
    // "value": "\"\\\\\\\\\\\\\\\"\""
    // 涵盖字符串开头的"和结尾的"
    // 对于需要转义的字符（包括头尾的"）增加转义符号
    // 去掉头尾的"
    // debug_out("--------------------no head end string");
    std::string value = raw_str.substr(1, raw_str.size()-2);
    // debug_out(value);
    // 注意到\\在string类型中是两个c风格字符'\\'

    // debug_out("--------------------1");
    /*
        value:      \n
        cstyle:     '\\'|'n'
        symbol:     \n
        replace:    '\n'
    */
    value = replace_escape_c(value, 'n');
    // debug_out(value);

    // debug_out("--------------------2");
    /*
        value:      \\
        cstyle:     '\\'|'\\'
        symbol:     \
        replace:    '\\'
    */
    value = replace_escape_c(value, '\\');
    // debug_out(value);

    // debug_out("--------------------3");
    /*
        value:      \'
        cstyle:     '\\'|'\''
        symbol:     \'
        replace:    '\''
    */
    value = replace_escape_c(value, '\'');
    // debug_out(value);

    // debug_out("--------------------4");
    /*
        value:      \"
        cstyle:     '\\'|'\"'
        symbol:     \"
        replace:    '\"'
    */
    value = replace_escape_c(value, '\"');
    // debug_out(value);
    // debug_out("--------------------end");
    return new StringLiteral(type, value);
}

UnaryExpr* JsonToASG::UnaryExprToASG(llvm::json::Object* object){
    // debug_out("UnaryExprToASG");
    Type type = getType(object);
    std::string opcode = object->getString("opcode").getValue().str();
    Expr* expr = ExprToASG(object->getArray("inner")->begin()->getAsObject());
    return new UnaryExpr(type, opcode, expr);
}

BinaryExpr* JsonToASG::BinaryExprToASG(llvm::json::Object* object){
    // debug_out("BinaryExprToASG");
    Type type = getType(object);
    std::string opcode = object->getString("opcode").getValue().str();
    Expr* lhs = ExprToASG(object->getArray("inner")->front().getAsObject());
    Expr* rhs = ExprToASG(object->getArray("inner")->back().getAsObject());
    return new BinaryExpr(type, opcode, lhs, rhs);    
}

ParenExpr* JsonToASG::ParenExprToASG(llvm::json::Object* object){
    // debug_out("ParenExprToASG");
    Type type = getType(object);
    Expr* expr = ExprToASG(object->getArray("inner")->begin()->getAsObject());
    return new ParenExpr(type, expr);
}

Decl* JsonToASG::findRef(llvm::json::Object* object){
    auto referenced_object = object->getObject("referencedDecl");
    int id = std::stoi(referenced_object->getString("id").getValue().str(), nullptr, 16);
    return symbol_table.find(id)->second;
}

DeclRefExpr* JsonToASG::DeclRefExprToASG(llvm::json::Object* object){
    // debug_out("DeclRefExprToASG");
    Type type = getType(object);
    Decl* ref = findRef(object);
    // 优化
    ref->is_promotable = false;
    return new DeclRefExpr(type, ref);
}

InitListExpr* JsonToASG::InitListExprToASG(llvm::json::Object* object){
    // debug_out("InitListExprToASG");
    Type type = getType(object);
    std::vector<Expr*> sub_init_exprs;

    if (auto array_filler = object->getArray("array_filler"))
    {
        for (auto it = array_filler->begin()+1; it != array_filler->end(); it++)
        {
            sub_init_exprs.emplace_back(ExprToASG(it->getAsObject()));
        }
        return new InitListExpr(type, sub_init_exprs);
    }
    
    if (auto inner = object->getArray("inner"))
    {
        for (auto& it: *inner)
        {
            sub_init_exprs.emplace_back(ExprToASG(it.getAsObject()));
        }
        return new InitListExpr(type, sub_init_exprs);
    }
    
    return nullptr;
}

// 多维 
// array:           a[2][3] ArraySubscriptExpr
// inner->index:    3       
// inner->inner:    a[2]    ArraySubscriptExpr
// inner->index:    2
// inner->inner:    a       DeclRefExpr
// sub_subscript_expr 
// 一维
// array:    a[2]    ArraySubscriptExpr
// inner->index:    2
// inner->inner:    a       DeclRefExpr
ArraySubscriptExpr* JsonToASG::ArraySubscriptExprToASG(llvm::json::Object* object){
    // debug_out("ArraySubscriptExprToASG");
    Type type = getType(object);
    auto inner = object->getArray("inner");
    // 下一个ArraySubscriptExpr需要取object的两层inner
    auto sub_expr_object = inner->begin()->getAsObject()->getArray("inner")->begin()->getAsObject();
    auto sub_expr = ExprToASG(sub_expr_object); // kind可能为DeclRefExpr或ArraySubscriptExpr
    // index
    auto index_object = (*inner)[1].getAsObject();
    auto index = ExprToASG(index_object);
    std::vector<Expr*> indexs;
    indexs.emplace_back(index);

    // 多维数组
    // 如果是多维数组，需要将父ArraySubscriptExprToASG的index传入子ArraySubscriptExprToASG，返回子ArraySubscriptExprToASG
    if (auto sub_subscript_expr = dynamic_cast<ArraySubscriptExpr*>(sub_expr))
    {
        sub_subscript_expr->indexs.emplace_back(index);
        return sub_subscript_expr;
    }

    return new ArraySubscriptExpr(type, sub_expr, indexs);
}

ImplicitCastExpr* JsonToASG::ImplicitCastExprToASG(llvm::json::Object* object){
    // debug_out("ImplicitCastExprToASG");
    Type type = getType(object);
    std::string cast_kind = object->getString("castKind").getValue().str();
    Expr *sub_expr = ExprToASG(object->getArray("inner")->begin()->getAsObject());
    return new ImplicitCastExpr(type, cast_kind, sub_expr);
}

CallExpr* JsonToASG::CallExprToASG(llvm::json::Object* object){
    // debug_out("CallExprToASG");
    Type type = getType(object);
    auto inner = object->getArray("inner");
    // ref
    auto referenced_object = inner->begin()->getAsObject()->getArray("inner")->begin()->getAsObject();
    Decl* ref = findRef(referenced_object);
    // 优化
    ref->is_promotable = false;
    // parms
    std::vector<Expr*> parms;
    for (auto it = inner->begin()+1; it != inner->end(); it++)
    {
        parms.emplace_back(ExprToASG(it->getAsObject()));
    }
    return new CallExpr(type, ref, parms);
}

// --------Stmt--------
CompoundStmt* JsonToASG::CompoundStmtToASG(llvm::json::Object* object){
    // debug_out("CompoundStmtToASG");
    if(object == nullptr) return nullptr;

    auto inner = object->getArray("inner");
    if(inner == nullptr) return new CompoundStmt();

    std::vector<Stmt*> stmts;
    for (auto& it: *inner)
    {
        auto it_object = it.getAsObject();
        if (auto stmt = dynamic_cast<Stmt*>(ObjToASG(it_object)))
        {
            // debug_out("CompoundStmt.inner: Stmt");
            stmts.emplace_back(stmt);
        }
        else if(auto expr = dynamic_cast<Expr*>(ObjToASG(it_object)))
        {
            // debug_out("CompoundStmt.inner: ExprStmt");
            stmts.emplace_back(ExprStmtToASG(it_object));
        }
    }
    return new CompoundStmt(stmts);
}

NullStmt* JsonToASG::NullStmtToASG(llvm::json::Object* object){
    // debug_out("NullStmtToASG");
    return new NullStmt();
}

ReturnStmt* JsonToASG::ReturnStmtToASG(llvm::json::Object* object){
    // debug_out("ReturnStmtToASG");
    if (auto inner = object->getArray("inner"))
    {
        auto ret_expr_object = inner->begin()->getAsObject();
        Expr* ret_expr = ExprToASG(ret_expr_object);
        return new ReturnStmt(ret_expr);
    }
    return new ReturnStmt();
}

DeclStmt* JsonToASG::DeclStmtToASG(llvm::json::Object* object){
    // debug_out("DeclStmtToASG");
    std::vector<Decl*> decls;
    auto inner = object->getArray("inner");
    for (auto& it: *inner)
    {
        decls.emplace_back(DeclToASG(it.getAsObject()));
    }
    return new DeclStmt(decls);
}

ExprStmt* JsonToASG::ExprStmtToASG(llvm::json::Object* object){
    // debug_out("ExprStmtToASG");
    return new ExprStmt(ExprToASG(object));
}

IfStmt* JsonToASG::IfStmtToASG(llvm::json::Object* object){
    // debug_out("IfStmtToASG");
    auto inner = object->getArray("inner");
    // condition
    Expr* condition = ExprToASG(inner->begin()->getAsObject());
    
    // if
    Stmt* if_stmt = nullptr;
    auto if_object = (*inner)[1].getAsObject();
    if (if_object->getString("kind").getValue().str().find("Stmt") != std::string::npos)
    {
        // if stmt; if {stmt}
        if_stmt = StmtToASG(if_object);
    }
    // if expr
    else if_stmt = ExprStmtToASG(if_object);
    
    // else
    Stmt* else_stmt = nullptr;
    // if (object->getObject("hasElse") != nullptr && object->getBoolean("hasElse"))
    if (object->getBoolean("hasElse"))
    {
        auto else_object = (*inner)[2].getAsObject();
        if (else_object->getString("kind").getValue().str().find("Stmt") != std::string::npos)
        {
            // else stmt; else {stmt}
            else_stmt = StmtToASG(else_object);
        }
        else else_stmt = ExprStmtToASG(else_object);
    }
    return new IfStmt(condition, if_stmt, else_stmt);
}

WhileStmt* JsonToASG::WhileStmtToASG(llvm::json::Object* object){
    // debug_out("WhileStmtToASG");
    auto inner = object->getArray("inner");
    // condition
    Expr* condition = ExprToASG(inner->begin()->getAsObject());
    // stmt
    Stmt* stmt = nullptr;
    auto stmt_object = (*inner)[1].getAsObject();
    if (stmt_object->getString("kind").getValue().str() == "CompoundStmt")
    {
        // while(){}
        stmt = StmtToASG(stmt_object);
    }
    else stmt = ExprStmtToASG(stmt_object);

    return new WhileStmt(condition, stmt);
}

DoStmt* JsonToASG::DoStmtToASG(llvm::json::Object* object){
    // debug_out("DoStmtToASG");
    auto inner = object->getArray("inner");
    // condition
    Expr* condition = ExprToASG((*inner)[1].getAsObject());
    // stmt
    Stmt* stmt = nullptr;
    auto stmt_object = inner->begin()->getAsObject();
    if (stmt_object->getString("kind").getValue().str() == "CompoundStmt")
    {
        // do{}
        stmt = StmtToASG(stmt_object);
    }
    else stmt = ExprStmtToASG(stmt_object);

    return new DoStmt(condition, stmt);
}

BreakStmt* JsonToASG::BreakStmtToASG(llvm::json::Object* object){
    // debug_out("BreakStmtToASG");
    return new BreakStmt();
}

ContinueStmt* JsonToASG::ContinueStmtToASG(llvm::json::Object* object){
    // debug_out("ContinueStmtToASG");
    return new ContinueStmt();
}

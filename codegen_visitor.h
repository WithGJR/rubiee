#ifndef __CODE_GEN_VISITOR_H__
#define __CODE_GEN_VISITOR_H__ 1

#include <memory>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "./include/KaleidoscopeJIT.h"
#include "ast.h"

namespace Rubiee {

class ASTNodeVisitor {

public:
    virtual ~ASTNodeVisitor() = default;
    virtual void visit(Expr &expr) = 0;
    virtual void visit(Statement &stmt) = 0;
    virtual void visit(IntConst &int_const) = 0;
    virtual void visit(BinaryExpr &binary_expr) = 0;
    virtual void visit(ComparisonExpr &comparison_expr) = 0;
    virtual void visit(Variable &var) = 0;
    virtual void visit(VariableAssignment &var_assignment) = 0;
    virtual void visit(FunctionCall &function_call) = 0;
    virtual void visit(FunctionPrototype &function_prototype) = 0;
    virtual void visit(TopLevelExpr &top_level_expr) = 0;
    virtual void visit(Function &function) = 0;
};

class CodeGenVisitor : public ASTNodeVisitor {

public:
    CodeGenVisitor();
    
    void visit(Expr &expr);
    void visit(Statement &stmt);
    void visit(IntConst &int_const);
    void visit(BinaryExpr &binary_expr);
    void visit(ComparisonExpr &comparison_expr);
    void visit(Variable &var);
    void visit(VariableAssignment &var_assignment);
    void visit(FunctionCall &function_call);
    void visit(FunctionPrototype &function_prototype);
    void visit(TopLevelExpr &top_level_expr);
    void visit(Function &function);

    void executeCode();

private:
    // LLVM-related variables
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;

    // Store the intermediate codegen result
    llvm::Value *generated_value;
    llvm::Function *generated_function;

    // Symbol table
    std::map<std::string, llvm::AllocaInst *> variables;

    // JIT
    std::unique_ptr<llvm::orc::KaleidoscopeJIT> jit;
    
    // Functions 
    std::map<std::string, llvm::Function*> stdlib_functions;
    llvm::BasicBlock *main_function;

    // Methods
    void initModule(std::unique_ptr<llvm::Module> &module, std::string module_name); 
    void initStandardLibraryFunctions();
    void initTopLevelExpr();
};

}

#endif
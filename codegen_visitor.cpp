#include "codegen_visitor.h"
#include "llvm/IR/Constants.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/ADT/STLExtras.h"

#include <iostream>

Rubiee::CodeGenVisitor::CodeGenVisitor() : builder(context) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    jit = llvm::make_unique<llvm::orc::KaleidoscopeJIT>();
    initModule(module, "jit");
    initStandardLibraryFunctions();
    initTopLevelExpr();
};

void Rubiee::CodeGenVisitor::initModule(std::unique_ptr<llvm::Module> &module, std::string module_name) {
    module = llvm::make_unique<llvm::Module>(module_name, context);
    module->setDataLayout(jit->getTargetMachine().createDataLayout());
}

void Rubiee::CodeGenVisitor::initStandardLibraryFunctions() {
    // void _puts(int num, ...)
    stdlib_functions["puts"] = llvm::Function::Create(
        llvm::FunctionType::get(
            llvm::Type::getVoidTy(context),
            std::vector<llvm::Type *>(
                1,
                llvm::Type::getInt32Ty(context)
            ),
            true
        ),
        llvm::Function::ExternalLinkage,
        "_puts",
        module.get()
    );
}

void Rubiee::CodeGenVisitor::initTopLevelExpr() {
    // All top level expression is placed at the main function
    llvm::Function *fn = llvm::Function::Create(
        llvm::FunctionType::get(
            llvm::Type::getVoidTy(context),
            std::vector<llvm::Type *>(0),
            false
        ),
        llvm::Function::ExternalLinkage,
        "main",
        module.get()
    );

    main_function = llvm::BasicBlock::Create(context, "entry", fn);
}

void Rubiee::CodeGenVisitor::executeCode() {
    // insert return instruction to the end of the main function
    builder.SetInsertPoint(main_function);
    builder.CreateRetVoid();

    // Execute main function
    jit->addModule(std::move(module));
    auto symbol = jit->findSymbol("main");
    void (*main_fn)() = (void (*)()) (intptr_t) symbol.getAddress();
    main_fn();
}

void Rubiee::CodeGenVisitor::visit(Expr &expr) {}
void Rubiee::CodeGenVisitor::visit(Statement &stmt) {}

void Rubiee::CodeGenVisitor::visit(IntConst &int_const) {
    generated_value = llvm::ConstantInt::get(
        context, 
        llvm::APInt(32, int_const.val, true)
    );
}

void Rubiee::CodeGenVisitor::visit(BinaryExpr &binary_expr) {
    llvm::Value *lhs, *rhs;

    (binary_expr.leftOperand)->accept(*this);
    lhs = generated_value;

    (binary_expr.rightOperand)->accept(*this);
    rhs = generated_value;

    if (!lhs || !rhs) {
        generated_value = nullptr;
        return;
    }

    switch(binary_expr.op) {
    case '+':
        generated_value = builder.CreateAdd(lhs, rhs, "add");
        break;
    case '-':
        generated_value = builder.CreateSub(lhs, rhs, "sub");
        break;
    case '*':
        generated_value = builder.CreateMul(lhs, rhs, "mul");
        break;
    }
}

void Rubiee::CodeGenVisitor::visit(Variable &var) {
    llvm::Value *variable = variables[var.name];
    if (!variable) {
        fprintf(stderr, "Variable `%s` is undefined.\n", var.name.c_str());
        generated_value = nullptr;
        return;
    }

    generated_value = builder.CreateLoad(variable, var.name.c_str());
}

void Rubiee::CodeGenVisitor::visit(VariableAssignment &var_assignment) {
    // variable is not defined yet
    if (variables.find(var_assignment.var->name) == variables.end()) {
        llvm::Function *current_function = builder.GetInsertBlock()->getParent();
        llvm::IRBuilder<> variable_builder(
            &current_function->getEntryBlock(),
            current_function->getEntryBlock().begin()
        );
        variables[var_assignment.var->name] = variable_builder.CreateAlloca(
            llvm::Type::getInt32Ty(context),
            0,
            var_assignment.var->name.c_str()
        );
    }

    llvm::Value *variable_pointer = variables[var_assignment.var->name];

    var_assignment.expr->accept(*this);
    llvm::Value *init_value = generated_value;

    generated_value = builder.CreateStore(init_value, variable_pointer);
}

void Rubiee::CodeGenVisitor::visit(FunctionCall &function_call) {
    llvm::Function *fn;

    // if is a standard library function
    if (stdlib_functions.find(function_call.callee) != stdlib_functions.end()) {
        fn = stdlib_functions[function_call.callee];
    }

    std::vector<llvm::Value *> args_value;

    for (unsigned i = 0; i < function_call.args.size(); i++) {
        function_call.args[i]->accept(*this);
        args_value.push_back(generated_value);
    }

    // Pass the number of variable arguments
    if (fn->isVarArg()) {
        unsigned var_arg_num = function_call.args.size() == 0 ? 
                               fn->arg_size() :
                               function_call.args.size() - ( fn->arg_size() - 1 );

        args_value.insert(
            args_value.begin() + (fn->arg_size() - 1),
            llvm::ConstantInt::get(
                context,
                llvm::APInt(32, var_arg_num, true)
            )
        );
    }
    
    generated_value = builder.CreateCall(fn, args_value);
}

void Rubiee::CodeGenVisitor::visit(FunctionPrototype &function_prototype) {
    std::vector<llvm::Type *> int_args(
        function_prototype.args.size(), 
        llvm::Type::getInt32Ty(context)
    );
    llvm::FunctionType *function_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context),
        int_args,
        false
    );
    generated_function = llvm::Function::Create(
        function_type,
        llvm::Function::ExternalLinkage,
        function_prototype.name,
        module.get()
    );
}

void Rubiee::CodeGenVisitor::visit(TopLevelExpr &top_level_expr) {
    builder.SetInsertPoint(main_function);
    top_level_expr.expr->accept(*this);
}

void Rubiee::CodeGenVisitor::visit(Function &function) {
    llvm::Function *fn;
    (function.proto)->accept(*this);

    fn = generated_function;
    if (!fn) {
        generated_function = nullptr;
        return;
    }

    llvm::BasicBlock *basic_block = llvm::BasicBlock::Create(context, "entry", fn);
    builder.SetInsertPoint(basic_block);

    function.body->accept(*this);
    llvm::Value *return_val = generated_value;
    if (return_val) {
        builder.CreateRet(return_val);
        return;
    } 
    
    // error
    fn->eraseFromParent();
    generated_function = nullptr;
}
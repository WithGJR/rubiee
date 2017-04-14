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
    main_function = llvm::Function::Create(
        llvm::FunctionType::get(
            llvm::Type::getVoidTy(context),
            std::vector<llvm::Type *>(0),
            false
        ),
        llvm::Function::ExternalLinkage,
        "main",
        module.get()
    );

    // main_function = llvm::BasicBlock::Create(context, "entry", fn);
    llvm::BasicBlock::Create(context, "entry", main_function);
}

void Rubiee::CodeGenVisitor::executeCode() {
    // insert return instruction to the end of the main function
    builder.SetInsertPoint( &(main_function->back()) );
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
    generated_value = llvm::ConstantInt::getSigned(
        llvm::Type::getInt32Ty(context),
        int_const.val
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

void Rubiee::CodeGenVisitor::visit(ComparisonExpr &comparison_expr) {
    llvm::Value *lhs, *rhs;

    (comparison_expr.leftOperand)->accept(*this);
    lhs = generated_value;

    (comparison_expr.rightOperand)->accept(*this);
    rhs = generated_value;

    if (!lhs || !rhs) {
        generated_value = nullptr;
        return;
    }

    if (comparison_expr.op == ">") {
        generated_value = builder.CreateICmpSGT(lhs, rhs, ">");
    } else if (comparison_expr.op == "<") {
        generated_value = builder.CreateICmpSLT(lhs, rhs, "<");
    } else if (comparison_expr.op == "==") {
        generated_value = builder.CreateICmpEQ(lhs, rhs, "==");
    } else if (comparison_expr.op == ">=") {
        generated_value = builder.CreateICmpSGE(lhs, rhs, ">=");
    } else if (comparison_expr.op == "<=") {
        generated_value = builder.CreateICmpSLE(lhs, rhs, "<=");
    }
}

void Rubiee::CodeGenVisitor::visit(IfExpr &if_expr) {
    if_expr.condition->accept(*this);
    llvm::Value *cond = generated_value;

    if (!cond) {
        generated_value = nullptr;
        return;
    }

    cond = builder.CreateICmpNE(
        cond,
        llvm::ConstantInt::getFalse(context),
        "ifcond"
    );

    llvm::Function *current_function = builder.GetInsertBlock()->getParent();

    llvm::BasicBlock *then_block = llvm::BasicBlock::Create(context, "then", current_function);
    llvm::BasicBlock *else_block = llvm::BasicBlock::Create(context, "else");
    llvm::BasicBlock *end_block = llvm::BasicBlock::Create(context, "end");

    builder.CreateCondBr(cond, then_block, else_block);

    // Generate code for `then_block`
    builder.SetInsertPoint(then_block);

    for (auto expr = if_expr.then_exprs.begin(); expr != if_expr.then_exprs.end(); ++expr) {
        (*expr)->accept(*this);
    }
    llvm::Value *then_value = generated_value;

    // `then_block` can be empty, use `0` as its return value
    if (if_expr.then_exprs.size() == 0) {
        then_value = llvm::ConstantInt::get(
            context, 
            llvm::APInt(32, 0, true)
        );
    }

    if (!then_value) {
        generated_value = nullptr;
        return;
    }

    builder.CreateBr(end_block);

    // Nested `if` can cause the then_block be changed, so we need to restore it
    then_block = builder.GetInsertBlock();

    // Generate code for `else_block`

    current_function->getBasicBlockList().push_back(else_block);
    builder.SetInsertPoint(else_block);

    for (auto expr = if_expr.else_exprs.begin(); expr != if_expr.else_exprs.end(); ++expr) {
        (*expr)->accept(*this);
    }
    llvm::Value *else_value = generated_value;

    // `else_block` can be empty, use `0` as its return value
    if (if_expr.else_exprs.size() == 0) {
        else_value = llvm::ConstantInt::get(
            context, 
            llvm::APInt(32, 0, true)
        );
    }

    if (!else_value) {
        generated_value = nullptr;
        return;
    }

    builder.CreateBr(end_block);

    // Nested `if` can cause the else_block be changed, so we need to restore it
    else_block = builder.GetInsertBlock();

    // Generate code for `end_block`

    current_function->getBasicBlockList().push_back(end_block);
    builder.SetInsertPoint(end_block);

    llvm::PHINode *phi_node = builder.CreatePHI(llvm::Type::getInt32Ty(context), 2, "if_val");
    phi_node->addIncoming(then_value, then_block);
    phi_node->addIncoming(else_value, else_block);

    generated_value = phi_node;
}

void Rubiee::CodeGenVisitor::visit(ForLoopExpr &for_loop_expr) {
    for_loop_expr.start_expr->accept(*this);

    llvm::Function *current_function = builder.GetInsertBlock()->getParent();
    llvm::BasicBlock *before_loop_body_block = llvm::BasicBlock::Create(context, "before_loop_body", current_function);
    llvm::BasicBlock *loop_body_block = llvm::BasicBlock::Create(context, "loop_body", current_function);
    llvm::BasicBlock *after_loop_body_block = llvm::BasicBlock::Create(context, "after_loop_body");

    builder.CreateBr(before_loop_body_block);
    builder.SetInsertPoint(before_loop_body_block);

    for_loop_expr.continue_condition->accept(*this);
    llvm::Value *cond = generated_value;

    if (!cond) {
        generated_value = nullptr;
        return;
    }

    cond = builder.CreateICmpEQ(
        cond,
        llvm::ConstantInt::getTrue(context),
        "loop_cond"
    );

    builder.CreateCondBr(cond, loop_body_block, after_loop_body_block);

    builder.SetInsertPoint(loop_body_block);
    for (auto expr = for_loop_expr.body_exprs.begin(); expr != for_loop_expr.body_exprs.end(); ++expr) {
        (*expr)->accept(*this);
    }

    for_loop_expr.step_expr->accept(*this);

    builder.CreateBr(before_loop_body_block);

    current_function->getBasicBlockList().push_back(after_loop_body_block);

    builder.SetInsertPoint(after_loop_body_block);    
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

    builder.CreateStore(init_value, variable_pointer);
    var_assignment.var->accept(*this);
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
    builder.SetInsertPoint( &(main_function->back()) );
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
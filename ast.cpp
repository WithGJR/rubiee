#include <utility>
#include "ast.h"
#include "codegen_visitor.h"

void Rubiee::Expr::accept(ASTNodeVisitor &visitor) {}
void Rubiee::Statement::accept(ASTNodeVisitor &visitor) {}

Rubiee::IntConst::IntConst(int val) : val(val) {};

void Rubiee::IntConst::accept(ASTNodeVisitor &visitor) {
    visitor.visit(*this);
}

Rubiee::Variable::Variable(std::string name) : name(name) {};

void Rubiee::Variable::accept(ASTNodeVisitor &visitor) {
    visitor.visit(*this);
}

Rubiee::VariableAssignment::VariableAssignment(Variable *var, Expr *expr) 
                                               : var(var), expr(expr) {};

void Rubiee::VariableAssignment::accept(ASTNodeVisitor &visitor) {
    visitor.visit(*this);
}

Rubiee::BinaryExpr::BinaryExpr(Expr *leftOperand, Expr *rightOperand, char op) 
                : leftOperand(leftOperand), rightOperand(rightOperand), op(op) {};

void Rubiee::BinaryExpr::accept(ASTNodeVisitor &visitor) {
    visitor.visit(*this);
}

Rubiee::ComparisonExpr::ComparisonExpr(Expr *leftOperand, Expr *rightOperand, std::string op) 
                                       : leftOperand(leftOperand), rightOperand(rightOperand), op(op) {};

void Rubiee::ComparisonExpr::accept(ASTNodeVisitor &visitor) {
    visitor.visit(*this);
}

Rubiee::FunctionCall::FunctionCall(std::string callee, std::vector<Expr*> args) 
                                   : callee(callee), args(args) {};

void Rubiee::FunctionCall::accept(ASTNodeVisitor &visitor) {
    visitor.visit(*this);
}

Rubiee::FunctionPrototype::FunctionPrototype(std::string name, std::vector<std::string> args) 
                                             : name(name), args(std::move(args)) {};

void Rubiee::FunctionPrototype::accept(ASTNodeVisitor &visitor) {
    visitor.visit(*this);
}

Rubiee::TopLevelExpr::TopLevelExpr(Expr *expr) : expr(expr) {};

void Rubiee::TopLevelExpr::accept(ASTNodeVisitor &visitor) {
    visitor.visit(*this);
}

Rubiee::Function::Function(FunctionPrototype *proto, Expr *body)
                           : proto(proto), body(body) {};

void Rubiee::Function::accept(ASTNodeVisitor &visitor) {
    visitor.visit(*this);
}
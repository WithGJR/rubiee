#ifndef __AST_H__
#define __AST_H__ 1

#include <string>
#include <vector>

namespace Rubiee {
  
class ASTNodeVisitor;

class ASTNode {
public:
  virtual ~ASTNode() = default;
  virtual void accept(ASTNodeVisitor &visitor) = 0;
};

class Expr : public ASTNode {
public:
  void accept(ASTNodeVisitor &visitor);
};

class Statement : public ASTNode {
public:
  void accept(ASTNodeVisitor &visitor);
};

class IntConst : public Expr {
public:
  IntConst(int val);
  void accept(ASTNodeVisitor &visitor);

  int val;
};

class BinaryExpr : public Expr {
public:
  BinaryExpr(Expr *leftOperand,
             Expr *rightOperand,
             char op);

  void accept(ASTNodeVisitor &visitor);

  Expr *leftOperand, *rightOperand;
  char op;
};

class ComparisonExpr : public Expr {
public:
  ComparisonExpr(Expr *leftOperand,
                 Expr *rightOperand,
                 std::string op);
  void accept(ASTNodeVisitor &visitor);

  Expr *leftOperand, *rightOperand;
  std::string op;
};

class IfExpr : public Expr {
public:
  IfExpr(Expr *condition, std::vector<Expr*> then_exprs, std::vector<Expr*> else_exprs);
  void accept(ASTNodeVisitor &visitor);

  Expr *condition;
  std::vector<Expr*> then_exprs;
  std::vector<Expr*> else_exprs;
};

class Variable : public Expr {
public:
  Variable(std::string name);
  void accept(ASTNodeVisitor &visitor);

  std::string name;
};

class VariableAssignment : public Expr {
public:
  VariableAssignment(Variable *var, Expr *expr);
  void accept(ASTNodeVisitor &visitor);

  Variable *var;
  Expr *expr;
};

class FunctionCall : public Expr {
public:
  FunctionCall(std::string callee, std::vector<Expr*> args);
  void accept(ASTNodeVisitor &visitor);

  std::string callee;
  std::vector<Expr*> args;
};

class FunctionPrototype : public Statement {
public:
  FunctionPrototype(std::string name, std::vector<std::string> args);
  void accept(ASTNodeVisitor &visitor);

  std::string name;
  std::vector<std::string> args;
};

class TopLevelExpr : public Statement {
public:
  TopLevelExpr(Expr *expr);
  void accept(ASTNodeVisitor &visitor);

  Expr *expr;
};

class Function : public Statement {
public:
  Function(FunctionPrototype *proto, Expr *body);
  void accept(ASTNodeVisitor &visitor);

  FunctionPrototype *proto;
  Expr *body;
};
  
}

#endif

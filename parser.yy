%skeleton "lalr1.cc"
%require "3.0"

%verbose

%defines
%define api.namespace {Rubiee}
%define parser_class_name {Parser}

%code requires{
   #include "ast.h"
   #include "driver.h"

   namespace Rubiee {
        class Lexer;
        class Driver;
   }
}

%param { Rubiee::Lexer &lexer }
%parse-param { Rubiee::Driver &driver }

%code{
  static int yylex(Rubiee::Parser::semantic_type *yylval,
                   Rubiee::Lexer &lexer);

  static std::vector<Rubiee::ASTNode*> *transformTopLevelExprsIntoFunctions(std::vector<Rubiee::ASTNode*> *nodes) {
        for (unsigned i = 0; i < nodes->size(); i++) {
                (*nodes)[i] = new Rubiee::TopLevelExpr(
                        static_cast<Rubiee::Expr*>( (*nodes)[i] )
                );
        }
        return nodes;
  }
}

%union {
  std::string *str_const;
  int int_const;

  Expr *expr;
  std::vector<ASTNode*> *nodes;
  std::vector<Expr*> *exprs;
}

%token <int_const> INT_CONST
%token <str_const> IDENTIFIER
%token ASSIGNMENT
%token COMMA
%token L_PAREN
%token R_PAREN

%left PLUS MINUS
%left MUL DIV

%type <nodes> exprs
%type <expr> expr
%type <exprs> args

%start top

%%

top : exprs { driver.set_nodes(transformTopLevelExprsIntoFunctions($1)); }
    ;

exprs   : expr { $$ = new std::vector<ASTNode*>(); $$->push_back($1); }
        | exprs expr { $$ = $1; $$->push_back($2); }
        ;

expr    : INT_CONST { $$ = new IntConst($1); }
        | expr PLUS expr { $$ = new BinaryExpr($1, $3, '+'); }
        | expr MINUS expr { $$ = new BinaryExpr($1, $3, '-'); }
        | expr MUL expr { $$ = new BinaryExpr($1, $3, '*'); }
        | IDENTIFIER L_PAREN args R_PAREN { $$ = new FunctionCall( *$1, std::move(*$3) ); }
        | IDENTIFIER { 
                $$ = new Variable(*$1); 
                delete $1;
          }
        | IDENTIFIER ASSIGNMENT expr { 
                $$ = new VariableAssignment(
                        new Variable(*$1),
                        $3
                ); 
                delete $1;
          }
        ;

args    : expr { $$ = new std::vector<Expr*>(); $$->push_back($1); }
        | args COMMA expr { $$ = $1; $$->push_back($3); }
        ;

%%

#include "lexer.h"

static int yylex(Rubiee::Parser::semantic_type *yylval,
                 Rubiee::Lexer &lexer) {
    return lexer.yylex(yylval);
}

void
Rubiee::Parser::error( const std::string &err_message )
{
   std::cerr << "Error: " << err_message << "\n";
}

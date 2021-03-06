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
%token IF
%token FOR
%token ELSE
%token END
%token SEMICOLON
%token ASSIGNMENT
%token COMMA
%token L_PAREN
%token R_PAREN

%left GREATER_THAN LESS_THAN EQUAL GREATER_THAN_OR_EQUAL LESS_THAN_OR_EQUAL
%left PLUS MINUS
%left MUL DIV

%type <nodes> nodes
%type <exprs> exprs
%type <expr> expr
%type <exprs> args

%start top

%%

top : nodes { driver.set_nodes(transformTopLevelExprsIntoFunctions($1)); }
    ;

nodes : exprs {
                $$ = new std::vector<ASTNode*>();
                for (unsigned i = 0; i < $1->size(); i++) {
                        $$->push_back( (*$1)[i] );
                }
        }
      ;

exprs   : expr { $$ = new std::vector<Expr*>(); $$->push_back($1); }
        | exprs expr { $$ = $1; $$->push_back($2); }
        ;

expr    : INT_CONST { $$ = new IntConst($1); }
        | expr PLUS expr { $$ = new BinaryExpr($1, $3, '+'); }
        | expr MINUS expr { $$ = new BinaryExpr($1, $3, '-'); }
        | expr MUL expr { $$ = new BinaryExpr($1, $3, '*'); }
        | expr GREATER_THAN_OR_EQUAL expr { $$ = new ComparisonExpr($1, $3, ">="); }
        | expr LESS_THAN_OR_EQUAL expr { $$ = new ComparisonExpr($1, $3, "<="); }
        | expr GREATER_THAN expr { $$ = new ComparisonExpr($1, $3, ">"); }
        | expr LESS_THAN expr { $$ = new ComparisonExpr($1, $3, "<"); }
        | expr EQUAL expr { $$ = new ComparisonExpr($1, $3, "=="); }
        | IF expr exprs END { 
                $$ = new IfExpr( 
                        $2, 
                        *$3, 
                        std::vector<Expr*>() 
                     ); 
          }
        | IF expr exprs ELSE exprs END { 
                $$ = new IfExpr(
                        $2, 
                        *$3, 
                        *$5
                     ); 
          }
        | FOR expr SEMICOLON expr SEMICOLON expr exprs END { 
                $$ = new ForLoopExpr($2, $4, $6, std::move(*$7)); 
          }
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

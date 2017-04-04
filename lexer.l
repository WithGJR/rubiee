%{

#include <string>
#include "parser.bison.hh"
#include "lexer.h"

typedef Rubiee::Parser::token token;

%}

%option c++
%option yyclass="Rubiee::Lexer"
%option noyywrap
%option outfile="flex_lexer.cc"

%%

">=" {
  return(token::GREATER_THAN_OR_EQUAL);
}

"<=" {
  return(token::LESS_THAN_OR_EQUAL);
}

">" {
  return(token::GREATER_THAN);
}

"<" {
  return(token::LESS_THAN);
}

"==" {
  return(token::EQUAL);
}

"+" {
  return(token::PLUS);
}

"-" {
  return(token::MINUS);
}

"*" {
  return(token::MUL);
}

"%" {
  return(token::DIV);
}

"=" {
  return(token::ASSIGNMENT);
}

"," {
  return(token::COMMA);
}

"(" {
  return(token::L_PAREN);
}

")" {
  return(token::R_PAREN);
}
 
0|[1-9][0-9]* {
  yylval->int_const = std::stoi(yytext);
  return(token::INT_CONST);
}

[a-zA-Z][a-zA-Z0-9]* {
  yylval->str_const = new std::string(yytext);
  return(token::IDENTIFIER);
}

%%

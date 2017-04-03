#include "lexer.h"

Rubiee::Lexer::Lexer(std::istream *in) 
                     : yyFlexLexer(in), yylval(nullptr) {}


int Rubiee::Lexer::yylex(Rubiee::Parser::semantic_type *l_val) {
    yylval = l_val;
    return yylex();
}
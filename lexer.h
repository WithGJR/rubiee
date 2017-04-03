#ifndef __LEXER_H__
#define __LEXER_H__ 1

#include "parser.bison.hh"

#if ! defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#undef  YY_DECL
#define YY_DECL int Rubiee::Lexer::yylex()

namespace Rubiee {

class Lexer : public yyFlexLexer {
public:
    Lexer(std::istream *in);

    int yylex();
    int yylex(Rubiee::Parser::semantic_type *l_val);

private:
    Rubiee::Parser::semantic_type *yylval;
};

}

#endif
#include <memory>
#include <vector>
#include "lexer.h"
#include "driver.h"
#include "codegen_visitor.h"

Rubiee::Driver::Driver() : nodes(nullptr) {}

void Rubiee::Driver::set_nodes(std::vector<ASTNode*> *n) {
    nodes = n;
}

void Rubiee::Driver::parse(std::istream &input) {
    Lexer lexer = Lexer(&input);
    std::unique_ptr<Parser> parser( new Parser(lexer, *this) );
    parser->parse();

    std::unique_ptr<CodeGenVisitor> codegen( new CodeGenVisitor() );
    for (unsigned i = 0; i < nodes->size(); i++) {
        ((*nodes)[i])->accept(*codegen);
    }
    codegen->executeCode();
}
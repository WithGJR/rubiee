#ifndef __DRIVER_H__
#define __DRIVER_H__ 1

#include <vector>
#include "ast.h"

namespace Rubiee {

class Driver {
public:
    Driver();

    void set_nodes(std::vector<ASTNode*> *n);
    void parse(std::istream &input);

private:
    std::vector<ASTNode*> *nodes;
};

}

#endif
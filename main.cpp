#include <fstream>
#include "driver.h"


int main(int argc, char **argv)
{

    std::ifstream source_file (argv[1], std::ifstream::in);

    Rubiee::Driver *driver = new Rubiee::Driver();
    driver->parse(source_file);

    source_file.close();
    return 0;
}

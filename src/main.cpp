#include <iostream>
#include "../include/cli.h"

int main(int argc, char** argv) {
    if (argc > 2) {
        std::cerr << "Uso: " << argv[0] << " [programa.bin]\n";
        return 1;
    }

    rv::Cli cli;
    cli.run(argc == 2 ? argv[1] : "");
    return 0;
}
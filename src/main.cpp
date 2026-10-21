// Racine de composition (phase A : squelette, l'interface Qt Quick arrive avec ENH-004).
#include "enh/version.hpp"

#include <iostream>

int main() {
    std::cout << "Enhance " << enh::versionString() << " ("
              << (ENH_GPU_BACKEND != 0 ? "pipeline GPU" : "pipeline simule") << ")\n";
    return 0;
}

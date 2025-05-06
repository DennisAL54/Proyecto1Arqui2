#include "PE.h"
#include "Interconnect.h"
#include <memory>

int main() {
    auto bus = std::make_shared<Interconnect>();

    auto pe0 = std::make_shared<PE>(0, 0x10, bus);
    auto pe1 = std::make_shared<PE>(1, 0x20, bus);
    auto pe2 = std::make_shared<PE>(2, 0x30, bus);

    bus->registerPE(0, pe0);
    bus->registerPE(1, pe1);
    bus->registerPE(2, pe2);

    pe0->loadFromFile("ejemplo_pe0.txt");
    pe1->loadFromFile("ejemplo_pe1.txt");
    pe2->loadFromFile("ejemplo_pe2.txt");

    // Ejecución por pasos
    bool more = true;
    while (more) {
        more = false;
        more |= pe0->step();
        more |= pe1->step();
        more |= pe2->step();
    }

    return 0;
}

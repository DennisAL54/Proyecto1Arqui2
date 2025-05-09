#include "PE.h"
#include "Interconnect.h"
#include <memory>

int main() {
    auto bus = std::make_shared<Interconnect>();

    auto pe0 = std::make_shared<PE>(0, 0x10, bus);
    auto pe1 = std::make_shared<PE>(1, 0x20, bus);
    auto pe2 = std::make_shared<PE>(2, 0x30, bus);
    auto pe3 = std::make_shared<PE>(2, 0x40, bus);
    auto pe4 = std::make_shared<PE>(2, 0x50, bus);
    auto pe5 = std::make_shared<PE>(2, 0x60, bus);
    auto pe6 = std::make_shared<PE>(2, 0x70, bus);
    auto pe7 = std::make_shared<PE>(2, 0x80, bus);

    std::vector<std::shared_ptr<PE>> allPEs = {pe0, pe1, pe2, pe3, pe4, pe5, pe6};

    // Registrar en el bus
    for (auto &pe : allPEs) {
        bus->registerPE(pe->getId(), pe);
    }

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
    for (auto& pe : allPEs) {
        std::cout << "=== PE " << int(pe->getId()) << " STATISTICS ===\n";
        std::cout << "Cycles run:       " << pe->getCycleCount() << "\n";
        std::cout << "Cache hits:       " << pe->getStatCacheHits() << "\n";
        std::cout << "Cache misses:     " << pe->getStatCacheMisses() << "\n";
        std::cout << "Bytes read:       " << pe->getStatReadBytes() << "\n";
        std::cout << "Bytes written:    " << pe->getStatWriteBytes() << "\n\n";
        std::cout << "Weighted bytes read:   " << pe->getStatWeightedReadBytes() << "\n";
        std::cout << "Weighted bytes written:" << pe->getStatWeightedWriteBytes() << "\n\n";
    }

    return 0;
}

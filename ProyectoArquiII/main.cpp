#include "PE.h"
#include "Interconnect.h"
#include <memory>

int main() {
    auto bus = std::make_shared<Interconnect>();
    // Crear 3 PEs: 0,1,2 (registra según tus tests)
    auto pe0 = std::make_shared<PE>(0,0x10,bus);
    auto pe1 = std::make_shared<PE>(1,0x20,bus);
    auto pe2 = std::make_shared<PE>(2,0x30,bus);
    bus->registerPE(0,pe0);
    bus->registerPE(1,pe1);
    bus->registerPE(2,pe2);
    // Cargar instrucciones
    pe0->loadFromFile("ejemplo_pe0.txt");
    pe1->loadFromFile("ejemplo_pe1.txt");
    pe2->loadFromFile("ejemplo_pe2.txt");
    // Ejecutar
    pe0->start(); pe1->start(); pe2->start();
    pe0->join();  pe1->join();  pe2->join();
    return 0;
}
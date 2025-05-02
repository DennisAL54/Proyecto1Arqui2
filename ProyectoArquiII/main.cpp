#include "PE.h"
#include "Interconnect.h"
#include <memory>

int main() {
    auto bus = std::make_shared<Interconnect>();

    auto pe0 = std::make_shared<PE>(0, 0x10, bus);
    auto pe1 = std::make_shared<PE>(1, 0x20, bus);

    bus->registerPE(0, pe0);
    bus->registerPE(1, pe1);

    Message instr = {
        .type = "WRITE_MEM",
        .src = 0,
        .dest = 1,
        .addr = 0x100,
        .size = 16,
        .cache_line = 0,
        .data = std::vector<uint8_t>(16, 0xAB),
        .qos = 0x10
    };

    pe0->loadInstructions({instr});
    pe0->start();
    pe1->start();

    pe0->join();
    pe1->join();

    return 0;
}

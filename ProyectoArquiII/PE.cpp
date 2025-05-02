#include "PE.h"
#include "Interconnect.h"  // include full definition of Interconnect here
#include <iostream>

PE::PE(uint8_t id, uint8_t qos_level, std::shared_ptr<Interconnect> bus)
    : pe_id(id), qos(qos_level), interconnect(bus), running(false)
{
    cache.resize(128, std::vector<uint8_t>(16, 0));
}

void PE::loadInstructions(const std::vector<Message>& instrs) {
    instructions = instrs;
}

void PE::start() {
    running = true;
    thread = std::thread(&PE::run, this);
}

void PE::join() {
    if (thread.joinable())
        thread.join();
}

void PE::run() {
    for (const auto& instr : instructions) {
        executeInstruction(instr);
    }
    running = false;
}

void PE::executeInstruction(const Message& instr) {
    std::cout << "[PE " << int(pe_id) << "] Ejecuta " << instr.type
              << " hacia PE " << int(instr.dest) << "\n";
    interconnect->sendMessage(instr);
}

void PE::receiveMessage(const Message& msg) {
    std::cout << "[PE " << int(pe_id) << "] Recibió mensaje tipo "
              << msg.type << " desde PE " << int(msg.src) << "\n";
}
#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <string>
#include "Message.h"

class Interconnect;
class PE {
public:
    PE(uint8_t id, uint8_t qos, std::shared_ptr<Interconnect> bus);

    // Carga instrucciones desde un texto o archivo
    void loadInstructions(const std::vector<Message>& instrs);
    void loadFromFile(const std::string& path);

    // Ejecución batch
    void start();
    void join();

    // Stepping: ejecutar una instrucción
    bool step();        // true si ejecutó, false si ya no hay más

    void receiveMessage(const Message& msg);
    uint8_t getId() const { return pe_id; }

private:
    void run();
    void executeInstruction(const Message& instr);

    uint8_t pe_id;
    uint8_t qos;
    std::vector<Message> instructions;
    size_t pc = 0;                     // contador de programa
    std::thread thread;
    std::atomic<bool> running;
    std::vector<std::vector<uint8_t>> cache;
    std::shared_ptr<Interconnect> interconnect;
};
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
    void loadFromFile(const std::string& path);
    void start();
    void join();
    void receiveMessage(const Message& msg);
    uint8_t getId() const { return pe_id; }
private:
    void run();
    void executeInstruction(const Message& instr);
    uint8_t pe_id;
    uint8_t qos;
    std::vector<Message> instructions;
    std::thread thread;
    std::atomic<bool> running;
    std::vector<std::vector<uint8_t>> cache;
    std::shared_ptr<Interconnect> interconnect;
};

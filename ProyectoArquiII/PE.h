#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <string>
#include <iostream>
#include <queue>
#include "Message.h"

struct CacheLine {
    std::vector<uint8_t> data;  // 16 bytes
    bool valid;
    uint32_t tag;

    CacheLine() : data(16, 0), valid(false), tag(0) {}
};

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

    void receiveMessage(const Message& msg) {
        std::lock_guard<std::mutex> lk(mtx_inbox);   // proteger cola
        inbox.push(msg);                             // ← NUEVO
    }
    uint8_t getId() const { return pe_id; }

private:
    void run();
    std::queue<Message> inbox;      // cola de mensajes entrantes
    std::mutex mtx_inbox;           // mutex para inbox
    bool stalled = false;           // si está esperando respuesta
    uint32_t waitingAddr = 0;
    void executeInstruction(const Message& instr);

    uint8_t pe_id;
    uint8_t qos;
    std::vector<Message> instructions;
    size_t pc = 0;                     // contador de programa
    std::thread thread;
    std::atomic<bool> running;
    std::vector<CacheLine> cache;
    std::shared_ptr<Interconnect> interconnect;

    bool processOneMessage() {
        std::lock_guard<std::mutex> lk(mtx_inbox);
        if (inbox.empty()) return false;
        Message msg = inbox.front();
        inbox.pop();

        if (msg.type == OpType::READ_RESP) {
            // cargar en caché y liberar stall
            writeToCache(msg.addr, msg.data);
            stalled = false;
            std::cout << "[PE " << int(pe_id) << "] READ_RESP recibido, cache actualizada, desbloqueado\n";
        }
        else if (msg.type == OpType::WRITE_RESP) {
            stalled = false;
            std::cout << "[PE " << int(pe_id) << "] WRITE_RESP recibido, desbloqueado\n";
        }
        // (luego procesar INV messages en el próximo paso)
        return true;
    }

    std::pair<int, uint32_t> getCacheIndexAndTag(uint32_t addr) {
        const int block_size = 16;
        int index = (addr / block_size) % 128;
        uint32_t tag = (addr / block_size) / 128;
        return { index, tag };
    }

    // NUEVO: comprueba hit o miss
    bool checkCacheHit(uint32_t addr) {
        auto [index, tag] = getCacheIndexAndTag(addr);
        return cache[index].valid && cache[index].tag == tag;
    }

    // NUEVO: lectura de datos desde caché
    std::vector<uint8_t> readFromCache(uint32_t addr) {
        auto [index, _] = getCacheIndexAndTag(addr);
        return cache[index].data;
    }

    // NUEVO: escritura de datos en caché
    void writeToCache(uint32_t addr, const std::vector<uint8_t>& data) {
        auto [index, tag] = getCacheIndexAndTag(addr);
        cache[index].valid = true;
        cache[index].tag = tag;
        cache[index].data = data;
    }
};
#include "PE.h"
#include "Interconnect.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Latencias (en ciclos)
static constexpr uint64_t LAT_CACHE_HIT    = 1;
static constexpr uint64_t LAT_CACHE_MISS   = 10;   // incluye tiempo de solicitar al bus
static constexpr uint64_t LAT_MEM_PER_BYTE = 1;    // adicional por byte transferido

PE::PE(uint8_t id, uint8_t qos_level, std::shared_ptr<Interconnect> bus)
    : pe_id(id), qos(qos_level), interconnect(bus), running(false)
{
    cache.resize(128);
}

void PE::loadInstructions(const std::vector<Message>& instrs) {
    instructions = instrs;
    pc = 0;
}

void PE::loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("No pude abrir " + path);
    std::string line;
    instructions.clear(); pc = 0;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string op;
        iss >> op;
        Message msg;
        if (op == "WRITE_MEM") msg.type = OpType::WRITE_MEM;
        else if (op == "READ_MEM") msg.type = OpType::READ_MEM;
        else if (op == "BROADCAST_INVALIDATE") msg.type = OpType::BROADCAST_INVALIDATE;
        else if (op == "INV_ACK") msg.type = OpType::INV_ACK;
        else if (op == "INV_COMPLETE") msg.type = OpType::INV_COMPLETE;
        else if (op == "READ_RESP") msg.type = OpType::READ_RESP;
        else if (op == "WRITE_RESP") msg.type = OpType::WRITE_RESP;
        else throw std::runtime_error("Opcode desconocido: " + op);

        std::string src_s, dest_s, addr_s, size_s, line_s, qos_s;
        iss >> src_s >> dest_s >> addr_s >> size_s >> line_s >> qos_s;
        msg.src        = static_cast<uint8_t>( std::stoul(src_s,  nullptr, 0) );
        msg.dest       = static_cast<uint8_t>( std::stoul(dest_s, nullptr, 0) );
        msg.addr       = static_cast<uint32_t>(std::stoul(addr_s, nullptr, 0));
        msg.size       = static_cast<uint32_t>(std::stoul(size_s, nullptr, 0));
        msg.cache_line = static_cast<uint32_t>(std::stoul(line_s, nullptr, 0));
        msg.qos        = static_cast<uint8_t>( std::stoul(qos_s, nullptr, 0) );

        msg.data.clear();
        std::string byteToken;
        while (iss >> byteToken) {
            msg.data.push_back(
                static_cast<uint8_t>( std::stoul(byteToken, nullptr, 16) )
            );
        }
        instructions.push_back(msg);
    }
}

void PE::start() {
    running = true;
    thread = std::thread(&PE::run, this);
}

void PE::join() {
    if (thread.joinable()) thread.join();
}

void PE::run() {
    while (step()) { /* ejecuta todo */ }
    running = false;
}

bool PE::step() {
    // 0) avanzar ciclos si hay latencia pendiente
    if (pendingLatency > 0) {
        cycleCount++;
        pendingLatency--;
        return true;  // consumió un ciclo
    }

    // 1) procesar un mensaje si hay
    if (processOneMessage()) return true;

    // 2) si está stalled esperando respuesta, solo avanza ciclo
    if (stalled) {
        cycleCount++;
        return true;
    }

    // 3) ejecutar siguiente instrucción
    if (pc < instructions.size()) {
        executeInstruction(instructions[pc]);
        pc++;
        return true;
    }
    return false;
}


void PE::executeInstruction(const Message& instr) {
    std::cout << "[PE " << int(pe_id) << "] Ejecuta "
              << to_string(instr.type)
              << " hacia PE " << int(instr.dest) << std::endl;

    //Manejo básico de hit/miss para READ_MEM y WRITE_MEM
    if (instr.type == OpType::READ_MEM) {
        uint32_t addr = instr.addr;
        if (checkCacheHit(addr)) {
            statCacheHits++;
            pendingLatency += LAT_CACHE_HIT;
            statReadBytes += instr.size;
            std::cout << "  → Cache HIT, +1 ciclo\n";
        } else {
            statCacheMisses++;
            pendingLatency += LAT_CACHE_MISS + instr.size * LAT_MEM_PER_BYTE;
            statReadBytes += instr.size;
            interconnect->sendMessage(instr);
            std::cout << "  → Cache MISS, +"
                      << LAT_CACHE_MISS + instr.size * LAT_MEM_PER_BYTE
                      << " ciclos\n";
        }
    }
    else if (instr.type == OpType::WRITE_MEM) {
        uint32_t addr = instr.addr;
        if (checkCacheHit(addr)) {
            statCacheHits++;
            pendingLatency += LAT_CACHE_HIT;
            statWriteBytes += instr.data.size();
            writeToCache(addr, instr.data);
            std::cout << "  → Write HIT, +1 ciclo\n";
        } else {
            statCacheMisses++;
            pendingLatency += LAT_CACHE_MISS + instr.data.size() * LAT_MEM_PER_BYTE;
            statWriteBytes += instr.data.size();
            writeToCache(addr, instr.data);
            interconnect->sendMessage(instr);
            std::cout << "  → Write MISS, +"
                      << LAT_CACHE_MISS + instr.data.size() * LAT_MEM_PER_BYTE
                      << " ciclos\n";
        }
    }
    else {
        // otros tipos de mensaje siguen igual
        interconnect->sendMessage(instr);
    }
}

bool PE::processOneMessage() {
    std::lock_guard<std::mutex> lk(mtx_inbox);
    if (inbox.empty()) return false;
    Message msg = inbox.front();
    inbox.pop();

    switch (msg.type) {
        case OpType::READ_RESP:
            writeToCache(msg.addr, msg.data);
            stalled = false;
            std::cout << "[PE " << int(pe_id) << "] READ_RESP recibido, cache actualizada\n";
            break;

        case OpType::WRITE_RESP:
            stalled = false;
            std::cout << "[PE " << int(pe_id) << "] WRITE_RESP recibido, desbloqueado\n";
            break;

        case OpType::BROADCAST_INVALIDATE: {
            // Invalidar línea si existe
            auto [idx, tag] = getCacheIndexAndTag(msg.addr);
            if (cache[idx].valid && cache[idx].tag == tag) {
                cache[idx].valid = false;
                std::cout << "[PE " << int(pe_id) << "] Invalidated cache line addr=" << msg.addr << "\n";
            }
            // Enviar INV_ACK de vuelta al emisor
            Message ack { OpType::INV_ACK, pe_id, msg.src, msg.addr, {}, qos };
            interconnect->sendMessage(ack);
            std::cout << "[PE " << int(pe_id) << "] Sent INV_ACK to PE " << int(msg.src) << "\n";
            break;
        }

        case OpType::INV_ACK: {
            // Recibimos ack de otro PE: reenviamos INV_COMPLETE al originador original
            // Suponemos msg.dest guarda el original requester
            Message complete { OpType::INV_COMPLETE, pe_id, msg.dest, msg.addr, {}, qos };
            interconnect->sendMessage(complete);
            std::cout << "[PE " << int(pe_id) << "] Sent INV_COMPLETE to PE " << int(msg.dest) << "\n";
            break;
        }

        case OpType::INV_COMPLETE:
            // Podríamos contabilizar completes para liberar un write pending; por ahora log
            std::cout << "[PE " << int(pe_id) << "] INV_COMPLETE recibido from PE " << int(msg.src) << "\n";
            break;

        default:
            // Otros mensajes ya manejados antes
            break;
    }
    return true;
}

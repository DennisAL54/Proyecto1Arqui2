#include "PE.h"
#include "Interconnect.h"
#include <fstream>
#include <sstream>
#include <iostream>

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
    // 1) procesar 1 mensaje si hay
    if (processOneMessage()) {
        return true;  // consumió mensaje, contamos como actividad
    }

    // 2) si está stancado esperando respuesta, no ejecutar instrucción
    if (stalled) {
        return !inbox.empty();
    }

    // 3) si quedan instrucciones, ejecutar la siguiente
    if (pc < instructions.size()) {
        const Message instr = instructions[pc];

        // Si es READ_MEM o WRITE_MEM y cache miss => stall
        if (instr.type == OpType::READ_MEM && !checkCacheHit(instr.addr)) {
            stalled = true;
            waitingAddr = instr.addr;
            // enviar la solicitud al bus
            interconnect->sendMessage(instr);
            std::cout << "[PE " << int(pe_id) << "] stall en READ_MEM addr=" << instr.addr << "\n";
        }
        else if (instr.type == OpType::WRITE_MEM && !checkCacheHit(instr.addr)) {
            stalled = true;
            waitingAddr = instr.addr;
            writeToCache(instr.addr, instr.data);   // write-allocate
            interconnect->sendMessage(instr);
            std::cout << "[PE " << int(pe_id) << "] stall en WRITE_MEM addr=" << instr.addr << "\n";
        }
        else {
            // hit o cualquier otro instr
            executeInstruction(instr);
        }

        pc++;
        return true;
    }

    return false;  // nada por hacer
}

void PE::executeInstruction(const Message& instr) {
    std::cout << "[PE " << int(pe_id) << "] Ejecuta "
              << to_string(instr.type)
              << " hacia PE " << int(instr.dest) << std::endl;

    // NUEVO: manejo básico de hit/miss para READ_MEM y WRITE_MEM
    if (instr.type == OpType::READ_MEM) {
        uint32_t addr = instr.addr;
        if (checkCacheHit(addr)) {
            std::cout << "  → Cache HIT en dirección " << addr << std::endl;
            auto datos = readFromCache(addr);
            // aquí podrías generar un READ_RESP inmediato si quisieras
        } else {
            std::cout << "  → Cache MISS en dirección " << addr << std::endl;
            // En un miss: forward al Interconnect para traer datos
            interconnect->sendMessage(instr);
        }
    }
    else if (instr.type == OpType::WRITE_MEM) {
        uint32_t addr = instr.addr;
        const auto& datos = instr.data;  // asumimos campo data presente
        if (checkCacheHit(addr)) {
            std::cout << "  → Cache HIT (write) en dirección " << addr << std::endl;
            writeToCache(addr, datos);
        } else {
            std::cout << "  → Cache MISS (write) en dirección " << addr << std::endl;
            // cargar línea, luego escribir
            writeToCache(addr, datos);
            interconnect->sendMessage(instr);
        }
    }
    else {
        // otros tipos de mensaje siguen igual
        interconnect->sendMessage(instr);
    }
}

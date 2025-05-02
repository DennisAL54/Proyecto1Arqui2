#include "PE.h"
#include "Interconnect.h"
#include <iostream>
#include <fstream>
#include <sstream>

PE::PE(uint8_t id, uint8_t qos_level, std::shared_ptr<Interconnect> bus)
    : pe_id(id), qos(qos_level), interconnect(bus), running(false) {
    cache.resize(128, std::vector<uint8_t>(16, 0));
}

void PE::loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("No pude abrir " + path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0]=='#') continue;
        std::istringstream iss(line);
        std::string op; iss >> op;
        Message msg;
        if (op=="WRITE_MEM") msg.type = OpType::WRITE_MEM;
        else if (op=="READ_MEM") msg.type = OpType::READ_MEM;
        else if (op=="BROADCAST_INVALIDATE") msg.type = OpType::BROADCAST_INVALIDATE;
        else if (op=="INV_ACK") msg.type = OpType::INV_ACK;
        else if (op=="INV_COMPLETE") msg.type = OpType::INV_COMPLETE;
        else if (op=="READ_RESP") msg.type = OpType::READ_RESP;
        else if (op=="WRITE_RESP") msg.type = OpType::WRITE_RESP;
        else throw std::runtime_error("Opcode desconocido: " + op);
        std::string src_s, dest_s, addr_s, size_s, line_s, qos_s;
        iss >> src_s >> dest_s >> addr_s >> size_s >> line_s >> qos_s;
        msg.src        = static_cast<uint8_t>( std::stoul(src_s,nullptr,0) );
        msg.dest       = static_cast<uint8_t>( std::stoul(dest_s,nullptr,0) );
        msg.addr       = static_cast<uint32_t>(std::stoul(addr_s,nullptr,0));
        msg.size       = static_cast<uint32_t>(std::stoul(size_s,nullptr,0));
        msg.cache_line = static_cast<uint32_t>(std::stoul(line_s,nullptr,0));
        msg.qos        = static_cast<uint8_t>( std::stoul(qos_s,nullptr,0) );
        msg.data.clear();
        std::string byteToken;
        while (iss >> byteToken) {
            msg.data.push_back(
                static_cast<uint8_t>(std::stoul(byteToken,nullptr,16))
            );
        }
        instructions.push_back(msg);
    }
}

void PE::start() {
    running = true;
    thread = std::thread(&PE::run,this);
}

void PE::join() {
    if (thread.joinable()) thread.join();
}

void PE::run() {
    for (auto &instr : instructions) executeInstruction(instr);
    running = false;
}

void PE::executeInstruction(const Message& instr) {
    std::cout << "[PE "<<int(pe_id)<<"] Ejecuta "
              << to_string(instr.type)
              << " hacia PE "<<int(instr.dest)<<"\n";
    interconnect->sendMessage(instr);
}

void PE::receiveMessage(const Message& msg) {
    std::cout << "[PE "<<int(pe_id)<<"] Recibió mensaje tipo "
              << to_string(msg.type)
              << " desde PE "<<int(msg.src)<<"\n";
}
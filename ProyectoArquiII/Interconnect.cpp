#include "Interconnect.h"
#include "PE.h"
#include <iostream>

void Interconnect::registerPE(uint8_t id, std::shared_ptr<PE> pe) {
    std::lock_guard<std::mutex> lock(mtx);
    pe_map[id] = pe;
}

void Interconnect::sendMessage(const Message& msg) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = pe_map.find(msg.dest);
    if (it != pe_map.end()) {
        it->second->receiveMessage(msg);
    } else {
        std::cerr << "[Interconnect] PE destino " << int(msg.dest) << " no encontrado.\n";
    }
}
//
// Created by dennis on 29/04/25.
//

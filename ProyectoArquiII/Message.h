#pragma once
#include <cstdint>
#include <vector>
#include <string>

// Tipos de operación soportados
enum class OpType : uint8_t {
    WRITE_MEM,
    READ_MEM,
    BROADCAST_INVALIDATE,
    INV_ACK,
    INV_COMPLETE,
    READ_RESP,
    WRITE_RESP
};

inline std::string to_string(OpType op) {
    switch(op) {
        case OpType::WRITE_MEM: return "WRITE_MEM";
        case OpType::READ_MEM: return "READ_MEM";
        case OpType::BROADCAST_INVALIDATE: return "BROADCAST_INVALIDATE";
        case OpType::INV_ACK: return "INV_ACK";
        case OpType::INV_COMPLETE: return "INV_COMPLETE";
        case OpType::READ_RESP: return "READ_RESP";
        case OpType::WRITE_RESP: return "WRITE_RESP";
    }
    return "UNKNOWN";
}

struct Message {
    OpType     type;
    uint8_t    src;
    uint8_t    dest;
    uint32_t   addr;
    uint32_t   size;
    uint32_t   cache_line;
    std::vector<uint8_t> data;
    uint8_t    qos;
};

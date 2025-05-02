#pragma once
#include <cstdint>
#include <vector>
#include <string>

struct Message {
    std::string type;     // WRITE_MEM, READ_MEM, etc.
    uint8_t src;
    uint8_t dest;
    uint32_t addr;
    uint32_t size;
    uint32_t cache_line;
    std::vector<uint8_t> data;
    uint8_t qos;
};

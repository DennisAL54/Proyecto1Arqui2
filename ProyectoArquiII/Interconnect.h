#pragma once
#include <map>
#include <mutex>
#include <memory>
#include "Message.h"

class PE;
class Interconnect {
public:
    void registerPE(uint8_t id, std::shared_ptr<PE> pe);
    void sendMessage(const Message& msg);
private:
    std::map<uint8_t, std::shared_ptr<PE>> pe_map;
    std::mutex mtx;
};
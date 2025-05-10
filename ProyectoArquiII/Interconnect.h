#pragma once

#include <queue>
#include <atomic>
#include <mutex>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <condition_variable>
#include "Message.h"

class PE;

class Interconnect {
public:
    Interconnect();
    ~Interconnect();

    void registerPE(uint8_t id, std::shared_ptr<PE> pe);
    void sendMessage(const Message& msg);

private:
    struct TimedMessage {
        Message msg;
        int qos;
        uint64_t order;

        bool operator<(const TimedMessage& other) const {
            if (qos == other.qos) return order > other.order; // FIFO entre iguales
            return qos < other.qos; // mayor QoS = mÃ¡s prioridad
        }
    };

    std::mutex mtx;
    std::condition_variable cv;
    std::map<uint8_t, std::shared_ptr<PE>> pe_map;
    std::map<uint8_t, std::priority_queue<TimedMessage>> messageQueues;

    std::atomic<bool> running;
    std::thread dispatcher;

    uint64_t globalOrderCounter = 0;

    void dispatchLoop();
    void simulateLatency(const Message& msg);
};
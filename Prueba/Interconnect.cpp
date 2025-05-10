#include "Interconnect.h"
#include "PE.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace std::chrono;

Interconnect::Interconnect() : running(true), dispatcher(&Interconnect::dispatchLoop, this) {}

Interconnect::~Interconnect() {
    running = false;
    cv.notify_all();
    if (dispatcher.joinable()) dispatcher.join();
}

void Interconnect::registerPE(uint8_t id, std::shared_ptr<PE> pe) {
    std::lock_guard<std::mutex> lock(mtx);
    pe_map[id] = pe;
}

void Interconnect::sendMessage(const Message& msg) {
    std::lock_guard<std::mutex> lock(mtx);

    Message copy = msg;
    auto now = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
    copy.timeSent = now;

    int qos = msg.qos;
    copy.deliveryTime = 0; // se define después de latencia

    TimedMessage timed = {copy, qos, globalOrderCounter++};
    messageQueues[msg.dest].push(timed);

    cv.notify_all();
}

void Interconnect::dispatchLoop() {
    while (running) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]() {
            for (const auto& [_, q] : messageQueues) {
                if (!q.empty()) return true;
            }
            return !running;
        });

        for (auto& [dest, queue] : messageQueues) {
            if (queue.empty()) continue;

            TimedMessage tm = queue.top();
            queue.pop();

            Message msg = tm.msg;

            lock.unlock();
            simulateLatency(msg);

            if (msg.dest == 255) {
                std::lock_guard<std::mutex> lock2(mtx);
                for (auto& [id, pe] : pe_map) {
                    if (id != msg.src) {
                        pe->receiveMessage(msg);
                    }
                }
            } else {
                std::shared_ptr<PE> target = nullptr;
                {
                    std::lock_guard<std::mutex> lock2(mtx);
                    auto it = pe_map.find(msg.dest);
                    if (it != pe_map.end()) {
                        target = it->second;
                    } else {
                        std::cerr << "[Interconnect] PE destino " << int(msg.dest) << " no encontrado.\n";
                    }
                }
                if (target) target->receiveMessage(msg);
            }

            lock.lock();
        }
    }
}

void Interconnect::simulateLatency(const Message& msg) {
    // Simula latencia de red por tamaño
    int base_latency_ns = 1000; // 1us
    int latency = base_latency_ns + msg.size * 10; // escala con tamaño
    std::this_thread::sleep_for(std::chrono::nanoseconds(latency));
}
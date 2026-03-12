#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <queue>
#include <vector>
#include <simulator/event.h>

class EventQueue {
public:
    void push(const Event& e);
    Event pop();
    const Event& top() const;

    bool empty() const;
    void clear();
    void printEventQueue() const;
    
private:
    struct Comparator {
        bool operator()(const Event& a, const Event& b) const;
    };

    std::priority_queue<Event,
                        std::vector<Event>,
                        Comparator> queue;
};

#endif
#include <simulator/event_queue.h>
#include <iostream>

bool EventQueue::Comparator::operator()(const Event& a, const Event& b) const {
    if (a.time != b.time)
        return a.time > b.time;   // min-heap behavior
    return a.delta > b.delta;
}

void EventQueue::push(const Event& e) {
    queue.push(e);
}

Event EventQueue::pop() {
    Event e = queue.top();
    queue.pop();
    return e;
}

const Event& EventQueue::top() const {
    return queue.top();
}

bool EventQueue::empty() const {
    return queue.empty();
}

void EventQueue::clear() {
    while (!queue.empty())
        queue.pop();
}

void EventQueue::printEventQueue() const {
    // Make a copy because priority_queue cannot be iterated
    std::priority_queue<Event,
                        std::vector<Event>,
                        Comparator> temp = queue;

    std::cout << "EventQueue contents:\n";

    while (!temp.empty()) {
        const Event& e = temp.top();

        if(e.type == Event::SIGNAL_UPDATE){
            // Adjust fields depending on your Event structure
        std::cout << "\ttime: " << e.time
                  << ", delta: " << e.delta
                  << ", Signal: " << e.signal->get_name()
                  << std::endl;
        }
        else if(e.type == Event::PROCESS_EXEC){
            // Adjust fields depending on your Event structure
                std::cout << "time: " << e.time
                  << ", delta: " << e.delta
                  << ", Process: " << e.process->getName()
                  << std::endl;
        }

        temp.pop();
    }
}
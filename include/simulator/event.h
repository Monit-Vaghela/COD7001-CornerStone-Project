#ifndef EVENT_H
#define EVENT_H

#include <circuitBuilder/core/process.h>
#include <circuitBuilder/core/signal.h>

struct Event {

    enum EventType {
        SIGNAL_UPDATE,
        PROCESS_EXEC
    };

    int time;
    int delta;
    EventType type;

    Signal* signal;
    bool value;

    Process* process;

    Event(int t, int d, Signal* s, bool v)
        : time(t), delta(d), type(SIGNAL_UPDATE),
          signal(s), value(v), process(nullptr) {}

    Event(int t, int d, Process* p)
        : time(t), delta(d), type(PROCESS_EXEC),
          signal(nullptr), value(false), process(p) {}
};

#endif
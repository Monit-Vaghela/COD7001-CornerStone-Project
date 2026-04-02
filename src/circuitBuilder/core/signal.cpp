#include <circuitBuilder/core/signal.h>

Signal::Signal(const std::string& name, SignalType type, bool initial_value, bool initialized)
    : name(name), type(type), value(initial_value), next_value(initial_value), initialized(initialized) {}

void Signal::set_next(bool val) {
    next_value = val;
}

void Signal::commit() {
    prev_value = value;      
    value = next_value;
}

bool Signal::get_value() const {
    return value;
}

const std::string& Signal::get_name() const {
    return name;
}

SignalType Signal::get_type() const {
    return type;
}

void Signal::add_sensitive(Process* p) {
    sensitive_list.push_back(p);
}

const std::vector<Process*>& Signal::get_sensitive_list() const {
    return sensitive_list;
}

void Signal::initializeSignal(){
    initialized = true;
}

bool Signal::get_prev_value() const {
    return prev_value;
}

bool Signal::is_rising_edge() const {
    return prev_value == false && value == true;
}

bool Signal::is_falling_edge() const {
    return prev_value == true && value == false;
}
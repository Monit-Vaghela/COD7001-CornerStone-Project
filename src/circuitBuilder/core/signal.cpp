#include <circuitBuilder/core/signal.h>

Signal::Signal(const std::string& name, bool initial_value)
    : name(name), value(initial_value), next_value(initial_value) {}

void Signal::set_next(bool val) {
    next_value = val;
}

void Signal::commit() {
    value = next_value;
}

bool Signal::get_value() const {
    return value;
}

const std::string& Signal::get_name() const {
    return name;
}

void Signal::add_sensitive(Process* p) {
    sensitive_list.push_back(p);
}

const std::vector<Process*>& Signal::get_sensitive_list() const {
    return sensitive_list;
}
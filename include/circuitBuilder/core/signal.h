#ifndef SIGNAL_H
#define SIGNAL_H

#include <string>
#include <vector>

class Process;

enum class SignalType {
    INPUT,
    OUTPUT,
    INTERMEDIATE
};

class Signal {
public:
    Signal(const std::string& name,
           SignalType type,
           bool initial_value = false,
           bool initialized = false);

    void set_next(bool val);
    void commit();

    bool get_value() const;
    const std::string& get_name() const;
    SignalType get_type() const;

    void add_sensitive(Process* p);
    const std::vector<Process*>& get_sensitive_list() const;

    bool get_prev_value() const;
    bool is_rising_edge() const;
    bool is_falling_edge() const;
    
    void initializeSignal();

    bool initialized;

private:
    std::string name;
    bool value;
    bool next_value;
    bool prev_value;

    SignalType type;

    std::vector<Process*> sensitive_list;
};

#endif
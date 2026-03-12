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
           bool initial_value = false);

    void set_next(bool val);
    void commit();

    bool get_value() const;
    const std::string& get_name() const;
    SignalType get_type() const;

    void add_sensitive(Process* p);
    const std::vector<Process*>& get_sensitive_list() const;

private:
    std::string name;
    bool value;
    bool next_value;

    SignalType type;

    std::vector<Process*> sensitive_list;
};

#endif
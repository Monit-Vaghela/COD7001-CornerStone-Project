#ifndef PROCESS_H
#define PROCESS_H
#include <string>

class Process {
public:
    enum class ProcessType {
        GATE,
        BEHAVIOUR,
        OTHER
    };

    virtual void execute() = 0;

    virtual ProcessType getType() const = 0;
    virtual std::string getName() const = 0;
    virtual ~Process() = default;
};

#endif
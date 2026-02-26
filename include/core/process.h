#ifndef PROCESS_H
#define PROCESS_H

class Process {
public:
    virtual void execute() = 0;
    virtual ~Process() = default;
};

#endif
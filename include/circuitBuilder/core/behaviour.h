#ifndef BEHAVIOUR_H
#define BEHAVIOUR_H

#include <vector>
#include <string>
#include <stdexcept>

#include <circuitBuilder/core/process.h>
#include <circuitBuilder/core/signal.h>

enum class EdgeType {
    RISING,
    FALLING
};

class Behaviour : public Process {
public:
    Behaviour(Signal* trigger, EdgeType type, Signal* in, Signal* output)
        : trigger(trigger), type(type), input(in), output(output)
    {
        behaviourId = "B" + std::to_string(behaviourCounter++);
    }

    void execute() override {

        bool edgeDetected = false;

        switch(type) {
            case EdgeType::RISING:
                edgeDetected = trigger->is_rising_edge();
                break;
            case EdgeType::FALLING:
                edgeDetected = trigger->is_falling_edge();
                break;

        }

        if(edgeDetected) output->set_next(input->get_value());
    }

    ProcessType getType() const override {
        return ProcessType::BEHAVIOUR;
    }

    std::string getName() const override {
        switch(type) {
            case EdgeType::RISING: return "RISING";
            case EdgeType::FALLING: return "FALLING";
        }
        return "UNKNOWN";
    }

    Signal* getInput() const {
        return {input};
    }

    Signal* getOutput() const {
        return {output};
    }

    Signal* getTrigger() const { 
        return trigger; 
    }
    
    const std::string& getId() const {
        return behaviourId;
    }

private:
    Signal* trigger;
    EdgeType type;
    Signal* input;
    Signal* output;

    std::string behaviourId;
    inline static int behaviourCounter = 0;
};

inline EdgeType stringToEdgeType(const std::string& name) {

    if(name == "RISING") return EdgeType::RISING;
    if(name == "FALLING") return EdgeType::FALLING;
    throw std::runtime_error("Unknown gate type: " + name);
}

#endif
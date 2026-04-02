#ifndef GATE_H
#define GATE_H

#include <vector>
#include <string>
#include <stdexcept>

#include <circuitBuilder/core/process.h>
#include <circuitBuilder/core/signal.h>

enum class GateType {
    AND,
    OR,
    XOR,
    XNOR,
    NOT,
    NAND,
    NOR,
    PASS
};

class Gate : public Process {
public:
    Gate(GateType type, const std::vector<Signal*>& inputs, Signal* output)
        : type(type), inputs(inputs), output(output), level(-1)
    {
        gateId = "G" + std::to_string(++gateCounter);
    }

    void execute() override {

        bool result = false;

        switch(type) {

            case GateType::AND:
                result = true;
                for(auto* s : inputs)
                    result &= s->get_value();
                break;

            case GateType::OR:
                result = false;
                for(auto* s : inputs)
                    result |= s->get_value();
                break;

            case GateType::NAND:
                result = true;
                for(auto* s : inputs)
                    result &= s->get_value();
                result = !result;
                break;

            case GateType::NOR:
                result = false;
                for(auto* s : inputs)
                    result |= s->get_value();
                result = !result;
                break;

            case GateType::XOR:
                result = inputs[0]->get_value() ^ inputs[1]->get_value();
                break;

            case GateType::XNOR:
                result = !(inputs[0]->get_value() ^ inputs[1]->get_value());
                break;

            case GateType::NOT:
                result = !inputs[0]->get_value();
                break;
            
            case GateType::PASS:
                result = inputs[0]->get_value();
        }

        output->set_next(result);
    }

    ProcessType getType() const override {
        return ProcessType::GATE;
    }

    std::string getName() const override {
        switch(type) {
            case GateType::AND: return "AND";
            case GateType::OR: return "OR";
            case GateType::NAND: return "NAND";
            case GateType::NOR: return "NOR";
            case GateType::XOR: return "XOR";
            case GateType::XNOR: return "XNOR";
            case GateType::NOT: return "NOT";
            case GateType::PASS: return "PASS";
        }
        return "UNKNOWN";
    }

    std::vector<Signal*> getInputs() const {
        return inputs;
    }

    std::vector<Signal*> getOutputs() const {
        return {output};
    }

    void setLevel(int gateLevel){
        level = gateLevel;
    }

    const int getLevel(){
        return level;
    }

    const std::string& getId() const {
        return gateId;
    }

private:
    GateType type;
    std::vector<Signal*> inputs;
    Signal* output;

    int level;
    std::string gateId;
    inline static int gateCounter = 0;
};

inline GateType stringToGateType(const std::string& name) {

    if(name == "and") return GateType::AND;
    if(name == "or") return GateType::OR;
    if(name == "nand") return GateType::NAND;
    if(name == "nor") return GateType::NOR;
    if(name == "xor") return GateType::XOR;
    if(name == "xnor") return GateType::XNOR;
    if(name == "not") return GateType::NOT;
    if(name == "pass") return GateType::PASS;
    throw std::runtime_error("Unknown gate type: " + name);
}

#endif
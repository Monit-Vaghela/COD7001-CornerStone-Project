#include <compiler/parser.h>
#include <iostream>
#include <cstdlib>

using namespace std;

Token Parser::peek() {
    return toks[pos];
}

// Consume: check type only
Token Parser::consume(TokenType expected, const string& errorMsg) {
    Token t = toks[pos];
    if (t.type != expected) {
        cerr << "Parse error on line " << t.line
             << ": " << errorMsg
             << " (got \"" << t.value << "\")\n";
        exit(1);
    }
    pos++;
    return t;
}

// Consume: check type AND value
Token Parser::consume(TokenType expected,
                      const string& expectedValue,
                      const string& errorMsg) {
    Token t = toks[pos];
    if (t.type != expected || t.value != expectedValue) {
        cerr << "Parse error on line " << t.line
             << ": " << errorMsg
             << " (got \"" << t.value << "\")\n";
        exit(1);
    }
    pos++;
    return t;
}

bool Parser::isGate() {
    return peek().type == TokenType::GATE;
}


vector<string> Parser::parseStringList() {
    vector<string> result;

    consume(TokenType::LBRACKET, "Expected '[' to open list");

    // Empty list edge case
    if (peek().type == TokenType::RBRACKET) {
        consume(TokenType::RBRACKET, "Expected ']'");
        return result;
    }

    // First element
    Token t = peek();
    if (t.type == TokenType::STRING || t.type == TokenType::IDENTIFIER) {
        result.push_back(t.value);
        pos++;
    } else {
        cerr << "Parse error on line " << t.line
             << ": expected a name inside list (got \"" << t.value << "\")\n";
        exit(1);
    }

    // Remaining elements separated by commas
    while (peek().type == TokenType::COMMA) {
        consume(TokenType::COMMA, "Expected ','");
        Token elem = peek();
        if (elem.type == TokenType::STRING || elem.type == TokenType::IDENTIFIER) {
            result.push_back(elem.value);
            pos++;
        } else {
            cerr << "Parse error on line " << elem.line
                 << ": expected a name after ',' (got \"" << elem.value << "\")\n";
            exit(1);
        }
    }

    consume(TokenType::RBRACKET, "Expected ']' to close list");
    return result;
}

shared_ptr<ExprNode> Parser::parseAtom() {
    Token t = peek();

    // Parenthesised sub-expression
    if (t.type == TokenType::LPAREN) {
        consume(TokenType::LPAREN, "Expected '('");
        auto inner = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return inner;
    }

    // Unary NOT
    if (t.type == TokenType::GATE && t.value == "not") {
        pos++;   // consume "not"
        auto operand = parseAtom();
        return ExprNode::makeUnary("not", operand);
    }

    if (t.type == TokenType::IDENTIFIER || t.type == TokenType::STRING) {
        pos++;
        return ExprNode::makeLeaf(t.value);
    }

    cerr << "Parse error on line " << t.line
         << ": unexpected token in expression: \"" << t.value << "\"\n";
    exit(1);
}

shared_ptr<ExprNode> Parser::parseExpression() {
    auto left = parseAtom();

    while (isGate() && peek().value != "not") {
        string gate = peek().value;
        pos++;   // consume the gate token
        auto right = parseAtom();
        left = ExprNode::makeBinary(gate, left, right);
    }

    return left;
}

EntityNode Parser::parseEntity() {
    EntityNode entity;

    consume(TokenType::KEYWORD, "entity", "Expected keyword 'entity'");
    entity.name = consume(TokenType::IDENTIFIER, "Expected entity name").value;
    consume(TokenType::LBRACE, "Expected '{' after entity name");

    consume(TokenType::KEYWORD, "inputs", "Expected keyword 'inputs'");
    consume(TokenType::COLON, "Expected ':' after 'inputs'");
    entity.inputs = parseStringList();
    consume(TokenType::SEMICOLON, "Expected ';' after inputs list");

    consume(TokenType::KEYWORD, "outputs", "Expected keyword 'outputs'");
    consume(TokenType::COLON, "Expected ':' after 'outputs'");
    entity.outputs = parseStringList();
    consume(TokenType::SEMICOLON, "Expected ';' after outputs list");

    consume(TokenType::RBRACE, "Expected '}' to close entity");

    return entity;
}

vector<SignalMapEntry> Parser::parseSignalMapping() {
    vector<SignalMapEntry> mapping;

    consume(TokenType::LBRACKET, "Expected '[' to open signalMapping");

    while (peek().type != TokenType::RBRACKET) {
        SignalMapEntry entry;

        Token sig = peek();
        if (sig.type == TokenType::STRING || sig.type == TokenType::IDENTIFIER) {
            entry.signal = sig.value;
            pos++;
        } else {
            cerr << "Parse error on line " << sig.line
                 << ": expected signal name in signalMapping\n";
            exit(1);
        }

        consume(TokenType::ARROW, "Expected '<=' in signalMapping");

        Token port = peek();
        if (port.type == TokenType::STRING || port.type == TokenType::IDENTIFIER) {
            entry.port = port.value;
            pos++;
        } else {
            cerr << "Parse error on line " << port.line
                 << ": expected port name in signalMapping\n";
            exit(1);
        }

        mapping.push_back(entry);

        if (peek().type == TokenType::COMMA) {
            pos++;
        }
    }

    consume(TokenType::RBRACKET, "Expected ']' to close signalMapping");
    return mapping;
}

BehaviourNode Parser::parseBehaviour() {
    BehaviourNode beh;

    Token lhs = peek();
    if (lhs.type == TokenType::IDENTIFIER || lhs.type == TokenType::STRING) {
        beh.outputSignal = lhs.value;
        pos++;
    } else {
        cerr << "Parse error on line " << lhs.line
             << ": expected signal name on left of '<=' in behaviour\n";
        exit(1);
    }

    consume(TokenType::ARROW, "Expected '<=' in behaviour assignment");

    beh.exprTree = parseExpression();

    consume(TokenType::SEMICOLON, "Expected ';' after behaviour expression");

    return beh;
}

EdgeBehaviourNode Parser::parseEdgeBehaviour() {
    EdgeBehaviourNode eb;

    consume(TokenType::KEYWORD, "if", "Expected 'if'");

    Token edgeKw = peek();
    if (edgeKw.type != TokenType::KEYWORD ||
        (edgeKw.value != "is_risingEdge" && edgeKw.value != "is_fallingEdge")) {
        cerr << "Parse error on line " << edgeKw.line
             << ": expected 'is_risingEdge' or 'is_fallingEdge'\n";
        exit(1);
    }
    eb.edgeType = (edgeKw.value == "is_risingEdge") ? "rising" : "falling";
    pos++;

    consume(TokenType::LPAREN, "Expected '(' after edge keyword");
    Token clk = peek();
    if (clk.type != TokenType::IDENTIFIER && clk.type != TokenType::STRING) {
        cerr << "Parse error on line " << clk.line << ": expected clock signal name\n";
        exit(1);
    }
    eb.clockSignal = clk.value;
    pos++;
    consume(TokenType::RPAREN, "Expected ')' after clock signal");

    consume(TokenType::LBRACE, "Expected '{' after edge condition");

    Token out = peek();
    if (out.type != TokenType::IDENTIFIER && out.type != TokenType::STRING) {
        cerr << "Parse error on line " << out.line << ": expected output signal\n";
        exit(1);
    }
    eb.outputSignal = out.value;
    pos++;

    consume(TokenType::ARROW, "Expected '<=' inside edge behaviour");

    Token in = peek();
    if (in.type != TokenType::IDENTIFIER && in.type != TokenType::STRING) {
        cerr << "Parse error on line " << in.line << ": expected input signal\n";
        exit(1);
    }
    eb.inputSignal = in.value;
    pos++;

    consume(TokenType::SEMICOLON, "Expected ';' after edge assignment");
    consume(TokenType::RBRACE, "Expected '}' to close edge behaviour");

    return eb;
}

ArchNode Parser::parseArch() {
    ArchNode arch;

    consume(TokenType::KEYWORD, "architecture", "Expected keyword 'architecture'");
    arch.name = consume(TokenType::IDENTIFIER, "Expected architecture name").value;
    consume(TokenType::LBRACE, "Expected '{' after architecture name");

    consume(TokenType::KEYWORD, "signals", "Expected keyword 'signals'");
    consume(TokenType::COLON, "Expected ':' after 'signals'");
    arch.signals = parseStringList();
    consume(TokenType::SEMICOLON, "Expected ';' after signals list");

    consume(TokenType::KEYWORD, "signalMapping", "Expected keyword 'signalMapping'");
    consume(TokenType::COLON, "Expected ':' after 'signalMapping'");
    arch.signalMapping = parseSignalMapping();
    consume(TokenType::SEMICOLON, "Expected ';' after signalMapping");

    consume(TokenType::KEYWORD, "behaviour", "Expected keyword 'behaviour'");
    consume(TokenType::LBRACE, "Expected '{' after 'behaviour'");

    while (peek().type != TokenType::RBRACE) {
        Token t = peek();
        if (t.type == TokenType::KEYWORD && t.value == "if") {
            arch.edgeBehaviours.push_back(parseEdgeBehaviour());
        } else if (t.type == TokenType::IDENTIFIER || t.type == TokenType::STRING) {
            arch.behaviours.push_back(parseBehaviour());
        } else {
            cerr << "Parse error on line " << t.line
                << ": unexpected token in behaviour block: \"" << t.value << "\"\n";
            exit(1);
        }
    }

    consume(TokenType::RBRACE, "Expected '}' to close behaviour");
    consume(TokenType::RBRACE, "Expected '}' to close architecture");

    return arch;
}

//  .tes sub-parser
ProcessNode Parser::parseProcess() {
    ProcessNode proc;

    consume(TokenType::KEYWORD, "process", "Expected keyword 'process'");
    consume(TokenType::LPAREN, "Expected '(' after 'process'");
    consume(TokenType::LBRACKET, "Expected '[' to open assignments");

    // Parse signal assignments until ]
    while (peek().type != TokenType::RBRACKET) {
        Token sig = peek();
        if (sig.type != TokenType::STRING && sig.type != TokenType::IDENTIFIER) {
            cerr << "Parse error on line " << sig.line
                 << ": expected signal name in process\n";
            exit(1);
        }
        string sigName = sig.value;
        pos++;

        consume(TokenType::ARROW, "Expected '<=' in process assignment");

        Token val = consume(TokenType::NUMBER, "Expected 0 or 1 after '<='");
        proc.assignments[sigName] = stoi(val.value);

        if (peek().type == TokenType::COMMA) {
            pos++;
        }
    }

    consume(TokenType::RBRACKET, "Expected ']' to close assignments");
    consume(TokenType::COMMA, "Expected ',' between assignments and timestamp");

    // Timestamp
    Token ts = consume(TokenType::NUMBER, "Expected timestamp number");
    proc.timestamp = stoi(ts.value);

    consume(TokenType::RPAREN, "Expected ')' to close process");
    consume(TokenType::SEMICOLON, "Expected ';' after process");

    return proc;
}

//  Top-level entry points

ProgramNode Parser::parseCir(const vector<Token>& tokens) {
    toks = tokens;
    pos  = 0;

    ProgramNode program;
    program.entity = parseEntity();
    program.arch   = parseArch();

    consume(TokenType::END_OF_FILE, "Expected end of file");

    return program;
}

SimulatorNode Parser::parseTes(const vector<Token>& tokens) {
    toks = tokens;
    pos  = 0;

    SimulatorNode sim;

    consume(TokenType::KEYWORD, "simulator", "Expected keyword 'simulator'");
    sim.name = consume(TokenType::IDENTIFIER, "Expected simulator name").value;
    consume(TokenType::LBRACE, "Expected '{' after simulator name");

    consume(TokenType::KEYWORD, "processes", "Expected keyword 'processes'");
    consume(TokenType::LBRACE, "Expected '{' after 'processes'");

    // Parse all process() calls
    while (peek().type == TokenType::KEYWORD && peek().value == "process") {
        sim.processes.push_back(parseProcess());
    }

    consume(TokenType::RBRACE, "Expected '}' to close processes");
    consume(TokenType::RBRACE, "Expected '}' to close simulator");

    consume(TokenType::END_OF_FILE, "Expected end of file");

    return sim;
}
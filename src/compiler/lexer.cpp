#include <compiler/lexer.h>
#include <iostream>
#include <set>

using namespace std;


static const set<string> KEYWORDS = {
    "entity",
    "architecture",
    "simulator",
    "inputs",
    "outputs",
    "signals",
    "signalMapping",
    "behaviour",
    "processes",
    "process",
    "if",
    "is_risingEdge",
    "is_fallingEdge"
};


static const set<string> GATES = {
    "and",
    "or",
    "not",
    "xor",
    "xnor"
};



bool Lexer::isIdentStart(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c == '_');
}

bool Lexer::isIdentChar(char c) {
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

bool Lexer::isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool Lexer::isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}



TokenType Lexer::classifyWord(const string& word) {
    if (KEYWORDS.count(word)) return TokenType::KEYWORD;
    if (GATES.count(word))    return TokenType::GATE;
    return TokenType::IDENTIFIER;
}


vector<Token> Lexer::tokenize(const string& source) {

    vector<Token> tokens;
    int i    = 0;                    // current position in source
    int line = 1;                    // current 1-based line number
    int n    = (int)source.size();

    while (i < n) {

        if (isWhitespace(source[i])) {
            if (source[i] == '\n') line++;
            i++;
            continue;
        }

        if (i + 1 < n && source[i] == '/' && source[i+1] == '/') {
            while (i < n && source[i] != '\n') i++;
            continue;
        }

        if (source[i] == '"') {
            i++;  // skip opening "
            string val;
            while (i < n && source[i] != '"') {
                val += source[i++];
            }
            if (i < n) i++;  // skip closing "
            tokens.push_back(Token(TokenType::STRING, val, line));
            continue;
        }

        if (i + 1 < n && source[i] == '<' && source[i+1] == '=') {
            tokens.push_back(Token(TokenType::ARROW, "<=", line));
            i += 2;
            continue;
        }

        if (isDigit(source[i])) {
            string num;
            while (i < n && isDigit(source[i])) {
                num += source[i++];
            }
            tokens.push_back(Token(TokenType::NUMBER, num, line));
            continue;
        }

        if (isIdentStart(source[i])) {
            string word;
            while (i < n && isIdentChar(source[i])) {
                word += source[i++];
            }
            tokens.push_back(Token(classifyWord(word), word, line));
            continue;
        }

        char c = source[i];
        switch (c) {
            case '{': tokens.push_back(Token(TokenType::LBRACE,    "{", line)); break;
            case '}': tokens.push_back(Token(TokenType::RBRACE,    "}", line)); break;
            case '[': tokens.push_back(Token(TokenType::LBRACKET,  "[", line)); break;
            case ']': tokens.push_back(Token(TokenType::RBRACKET,  "]", line)); break;
            case '(': tokens.push_back(Token(TokenType::LPAREN,    "(", line)); break;
            case ')': tokens.push_back(Token(TokenType::RPAREN,    ")", line)); break;
            case ':': tokens.push_back(Token(TokenType::COLON,     ":", line)); break;
            case ';': tokens.push_back(Token(TokenType::SEMICOLON, ";", line)); break;
            case ',': tokens.push_back(Token(TokenType::COMMA,     ",", line)); break;

            default:
                // Anything else is unknown
                cerr << "Lexer warning: unknown character '" << c
                     << "' on line " << line << "\n";
                tokens.push_back(Token(TokenType::UNKNOWN, string(1,c), line));
                break;
        }
        i++;
    }

    tokens.push_back(Token(TokenType::END_OF_FILE, "", line));
    return tokens;
}
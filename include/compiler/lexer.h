#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    KEYWORD,
    GATE,
    IDENTIFIER,
    STRING,
    NUMBER,
    ARROW,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    LPAREN,
    RPAREN,
    COLON,
    SEMICOLON,
    COMMA,
    END_OF_FILE,
    UNKNOWN
};

// ─────────────────────────────────────────────
//  Token
// ─────────────────────────────────────────────
struct Token {
    TokenType   type;
    std::string value;   
    int line; 

    Token(TokenType t, const std::string& v, int l)
        : type(t), value(v), line(l) {}
};

// ─────────────────────────────────────────────
//  Lexer
// ─────────────────────────────────────────────
class Lexer {
public:
    // The last token is always END_OF_FILE.
    // On unrecognised input an UNKNOWN token is inserted (not a hard crash)
    std::vector<Token> tokenize(const std::string& source);

private:
    bool isIdentStart(char c);   // true for [a-zA-Z_]
    bool isIdentChar(char c);    // true for [a-zA-Z0-9_]
    bool isDigit(char c);        // true for [0-9]
    bool isWhitespace(char c);   // true for space / tab / \r / \n
    TokenType classifyWord(const std::string& word);
};

#endif 
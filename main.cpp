#include <iostream>
#include <stdlib.h>
#include <vector>
#include <fstream>
#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include <cctype>


using namespace std;

string loadFile(const std::string& filepath) {
    ifstream file(filepath);
    if (!file.is_open()) {
        throw runtime_error("Error: No se pudo abrir el archivo " + filepath);
    }
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

enum TokenType {
    IDENTIFIER,
    STRING,
    NUMBER,
    OPERATOR,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    COMMA,
    EQUALS,
    COLON,
    SEMICOLON,
    COMMENT
};

struct Token {
    TokenType type;
    string value;
};

class Lexer {
private:
    string source_;
    size_t current_pos_;

public:
    Lexer( const string& source ) : source_(source), current_pos_(0) {}
    
    vector<Token> tokenize() {
        vector<Token> tokens;

        while (current_pos_ < source_.length()) {
            char current_char = source_[current_pos_];

            // Espacios en blanco - los ignoramos
            if (isspace(current_char)) {
                current_pos_++;
                continue;
            }

            // Comentarios de línea (//)
            if (current_char == '/') {
                if (current_pos_ + 1 < source_.length() && source_[current_pos_ + 1] == '/') {
                    string comment;
                    while (current_pos_ < source_.length() && source_[current_pos_] != '\n') {
                        comment += source_[current_pos_];
                        current_pos_++;
                    }
                    tokens.push_back({COMMENT, comment});
                    cout << "Token COMMENT: " << comment << endl;
                    continue;
                }
                else {
                    tokens.push_back({OPERATOR, "/"});
                    cout << "Token OPERATOR: /" << endl;
                    current_pos_++;
                    continue;
                }
            }

            // Strings (entre comillas dobles)
            if (current_char == '"') {
                string string_value;
                string_value += current_char;
                current_pos_++; // Avanzar después de la comilla inicial
                
                while (current_pos_ < source_.length() && source_[current_pos_] != '"') {
                    string_value += source_[current_pos_];
                    current_pos_++;
                }
                
                if (current_pos_ < source_.length()) {
                    string_value += current_char; // Agregar la comilla final
                    current_pos_++; // Avanzar después de la comilla final
                }
                
                tokens.push_back({STRING, string_value});
                cout << "Token STRING: " << string_value << endl;
                continue;
            }

            // Números (enteros y decimales)
            if (isdigit(current_char)) {
                string number;
                bool has_dot = false;
                
                while (current_pos_ < source_.length()) {
                    char c = source_[current_pos_];
                    if (isdigit(c)) {
                        number += c;
                    } else if (c == '.' && !has_dot) {
                        has_dot = true;
                        number += c;
                    } else {
                        break;
                    }
                    current_pos_++;
                }
                
                tokens.push_back({NUMBER, number});
                cout << "Token NUMBER: " << number << endl;
                continue;
            }

            // Identificadores (letras, números y guiones bajos)
            if (isalpha(current_char) || current_char == '_') {
                string identifier;
                while (current_pos_ < source_.length() && (isalnum(source_[current_pos_]) || source_[current_pos_] == '_')) {
                    identifier += source_[current_pos_];
                    current_pos_++;
                }
                tokens.push_back({IDENTIFIER, identifier});
                cout << "Token IDENTIFIER: " << identifier << endl;
                continue;
            }

            // Operador de asignación (=)
            if (current_char == '=') {
                tokens.push_back({EQUALS, "="});
                cout << "Token EQUALS: =" << endl;
                current_pos_++;
                continue;
            }

            // Dos puntos (:)
            if (current_char == ':') {
                tokens.push_back({COLON, ":"});
                cout << "Token COLON: :" << endl;
                current_pos_++;
                continue;
            }

            // Punto y coma (;)
            if (current_char == ';') {
                tokens.push_back({SEMICOLON, ";"});
                cout << "Token SEMICOLON: ;" << endl;
                current_pos_++;
                continue;
            }

            // Llaves
            if (current_char == '{') {
                tokens.push_back({LBRACE, "{"});
                cout << "Token LBRACE: {" << endl;
                current_pos_++;
                continue;
            }

            if (current_char == '}') {
                tokens.push_back({RBRACE, "}"});
                cout << "Token RBRACE: }" << endl;
                current_pos_++;
                continue;
            }

            // Corchetes
            if (current_char == '[') {
                tokens.push_back({LBRACKET, "["});
                cout << "Token LBRACKET: [" << endl;
                current_pos_++;
                continue;
            }

            if (current_char == ']') {
                tokens.push_back({RBRACKET, "]"});
                cout << "Token RBRACKET: ]" << endl;
                current_pos_++;
                continue;
            }

            // Coma
            if (current_char == ',') {
                tokens.push_back({COMMA, ","});
                cout << "Token COMMA: ," << endl;
                current_pos_++;
                continue;
            }

            // Otros operadores básicos
            if (current_char == '+' || current_char == '-' || current_char == '*' || 
                current_char == '%' || current_char == '!') {
                tokens.push_back({OPERATOR, string(1, current_char)});
                cout << "Token OPERATOR: " << current_char << endl;
                current_pos_++;
                continue;
            }

            // Saltamos caracteres no reconocidos (no imprimimos mensaje de error por ahora)
            current_pos_++;
        }

        return tokens;
    }
};

int main() {
    try {
        string contenido = loadFile("Tetris.brik");
        Lexer lexer(contenido);
        vector<Token> tokens = lexer.tokenize();

        cout << "\n=== RESUMEN ===" << endl;
        cout << "Total de tokens reconocidos: " << tokens.size() << endl;
        cout << "\nTipos de tokens encontrados:" << endl;
        
        map<string, int> token_counts;
        for (const Token& token : tokens) {
            string token_name;
            switch (token.type) {
                case IDENTIFIER: token_name = "IDENTIFIER"; break;
                case STRING: token_name = "STRING"; break;
                case NUMBER: token_name = "NUMBER"; break;
                case OPERATOR: token_name = "OPERATOR"; break;
                case LBRACE: token_name = "LBRACE"; break;
                case RBRACE: token_name = "RBRACE"; break;
                case LBRACKET: token_name = "LBRACKET"; break;
                case RBRACKET: token_name = "RBRACKET"; break;
                case COMMA: token_name = "COMMA"; break;
                case EQUALS: token_name = "EQUALS"; break;
                case COLON: token_name = "COLON"; break;
                case SEMICOLON: token_name = "SEMICOLON"; break;
                case COMMENT: token_name = "COMMENT"; break;
            }
            token_counts[token_name]++;
        }

        for (const auto& pair : token_counts) {
            cout << "  " << pair.first << ": " << pair.second << " tokens" << endl;
        }

        cout << "\n=== PRIMEROS 30 TOKENS ===" << endl;
        int count = 0;
        for (const Token& token : tokens) {
            if (count >= 30) break;
            string token_name;
            switch (token.type) {
                case IDENTIFIER: token_name = "IDENTIFIER"; break;
                case STRING: token_name = "STRING"; break;
                case NUMBER: token_name = "NUMBER"; break;
                case OPERATOR: token_name = "OPERATOR"; break;
                case LBRACE: token_name = "LBRACE"; break;
                case RBRACE: token_name = "RBRACE"; break;
                case LBRACKET: token_name = "LBRACKET"; break;
                case RBRACKET: token_name = "RBRACKET"; break;
                case COMMA: token_name = "COMMA"; break;
                case EQUALS: token_name = "EQUALS"; break;
                case COLON: token_name = "COLON"; break;
                case SEMICOLON: token_name = "SEMICOLON"; break;
                case COMMENT: token_name = "COMMENT"; break;
            }
            cout << token_name << "(\"" << token.value << "\")" << endl;
            count++;
        }

    } catch (const std::runtime_error& e) {
        cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}

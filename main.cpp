// -----------------------------------------------------------------------------
// Analizador .brik — Lexer + Parser + AST + Serializador 1.0
// CLI:
//   analizador input.brik [-o arbol.ast] [--tokens] [--tokfile tokens.txt] [--no-ast]
// -----------------------------------------------------------------------------
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

string cargarArchivo(const string& ruta) {
    ifstream archivo(ruta);
    if (!archivo.is_open()) {
        throw runtime_error("Error: No se pudo abrir el archivo " + ruta);
    }
    stringstream buffer;
    buffer << archivo.rdbuf();
    return buffer.str();
}

enum TipoToken {
    IDENTIFICADOR,
    CADENA,
    NUMERO,
    OPERADOR,
    LLAVE_ABIERTA,
    LLAVE_CERRADA,
    CORCHETE_ABIERTO,
    CORCHETE_CERRADO,
    COMA,
    IGUAL,
    DOS_PUNTOS,
    PUNTO_Y_COMA,
    COMENTARIO
};

struct Token {
    TipoToken tipo;
    string valor;
};

// ----- LEXER -----

class AnalizadorLexico {
private:
    string fuente_;
    size_t pos_actual_;

public:
    AnalizadorLexico(const string& fuente) : fuente_(fuente), pos_actual_(0) {}

    vector<Token> tokenizar() {
        vector<Token> tokens;

        while (pos_actual_ < fuente_.length()) {
            char caracter_actual = fuente_[pos_actual_];

            // Espacios en blanco - los ignoramos
            if (isspace(caracter_actual)) {
                pos_actual_++;
                continue;
            }

            // Comentarios de línea (//)
            if (caracter_actual == '/') {
                if (pos_actual_ + 1 < fuente_.length() && fuente_[pos_actual_ + 1] == '/') {
                    string comentario;
                    while (pos_actual_ < fuente_.length() && fuente_[pos_actual_] != '\n') {
                        comentario += fuente_[pos_actual_];
                        pos_actual_++;
                    }
                    tokens.push_back({COMENTARIO, comentario});
                    cout << "TOKEN COMENTARIO: " << comentario << endl;
                    continue;
                }
                else {
                    tokens.push_back({OPERADOR, "/"});
                    cout << "TOKEN OPERADOR: /" << endl;
                    pos_actual_++;
                    continue;
                }
            }

            // Cadenas (entre comillas dobles)
            if (caracter_actual == '"') {
                string valor_cadena;
                valor_cadena += caracter_actual;
                pos_actual_++; // Avanzar después de la comilla inicial
                
                while (pos_actual_ < fuente_.length() && fuente_[pos_actual_] != '"') {
                    valor_cadena += fuente_[pos_actual_];
                    pos_actual_++;
                }
                
                if (pos_actual_ < fuente_.length()) {
                    valor_cadena += caracter_actual; // Agregar la comilla final
                    pos_actual_++; // Avanzar después de la comilla final
                }
                
                tokens.push_back({CADENA, valor_cadena});
                cout << "TOKEN CADENA: " << valor_cadena << endl;
                continue;
            }

            // Números (enteros y decimales)
            if (isdigit(caracter_actual)) {
                string numero;
                bool tiene_punto = false;
                
                while (pos_actual_ < fuente_.length()) {
                    char c = fuente_[pos_actual_];
                    if (isdigit(c)) {
                        numero += c;
                    } else if (c == '.' && !tiene_punto) {
                        tiene_punto = true;
                        numero += c;
                    } else {
                        break;
                    }
                    pos_actual_++;
                }
                
                tokens.push_back({NUMERO, numero});
                cout << "TOKEN NUMERO: " << numero << endl;
                continue;
            }

            // Identificadores (letras, números y guiones bajos)
            if (isalpha(caracter_actual) || caracter_actual == '_') {
                string identificador;
                while (pos_actual_ < fuente_.length() && (isalnum(fuente_[pos_actual_]) || fuente_[pos_actual_] == '_')) {
                    identificador += fuente_[pos_actual_];
                    pos_actual_++;
                }
                tokens.push_back({IDENTIFICADOR, identificador});
                cout << "TOKEN IDENTIFICADOR: " << identificador << endl;
                continue;
            }

            // Operador de asignación (=)
            if (caracter_actual == '=') {
                tokens.push_back({IGUAL, "="});
                cout << "TOKEN IGUAL: =" << endl;
                pos_actual_++;
                continue;
            }

            // Dos puntos (:)
            if (caracter_actual == ':') {
                tokens.push_back({DOS_PUNTOS, ":"});
                cout << "TOKEN DOS_PUNTOS: :" << endl;
                pos_actual_++;
                continue;
            }

            // Punto y coma (;)
            if (caracter_actual == ';') {
                tokens.push_back({PUNTO_Y_COMA, ";"});
                cout << "TOKEN PUNTO_Y_COMA: ;" << endl;
                pos_actual_++;
                continue;
            }

            // Llaves
            if (caracter_actual == '{') {
                tokens.push_back({LLAVE_ABIERTA, "{"});
                cout << "TOKEN LLAVE_ABIERTA: {" << endl;
                pos_actual_++;
                continue;
            }

            if (caracter_actual == '}') {
                tokens.push_back({LLAVE_CERRADA, "}"});
                cout << "TOKEN LLAVE_CERRADA: }" << endl;
                pos_actual_++;
                continue;
            }

            // Corchetes
            if (caracter_actual == '[') {
                tokens.push_back({CORCHETE_ABIERTO, "["});
                cout << "TOKEN CORCHETE_ABIERTO: [" << endl;
                pos_actual_++;
                continue;
            }

            if (caracter_actual == ']') {
                tokens.push_back({CORCHETE_CERRADO, "]"});
                cout << "TOKEN CORCHETE_CERRADO: ]" << endl;
                pos_actual_++;
                continue;
            }

            // Coma
            if (caracter_actual == ',') {
                tokens.push_back({COMA, ","});
                cout << "TOKEN COMA: ," << endl;
                pos_actual_++;
                continue;
            }

            // Otros operadores básicos
            if (caracter_actual == '+' || caracter_actual == '-' || caracter_actual == '*' || 
                caracter_actual == '%' || caracter_actual == '!') {
                tokens.push_back({OPERADOR, string(1, caracter_actual)});
                cout << "TOKEN OPERADOR: " << caracter_actual << endl;
                pos_actual_++;
                continue;
            }

            // Saltamos caracteres no reconocidos (no imprimimos mensaje de error por ahora)
            pos_actual_++;
        }

        return tokens;
    }
};

// ----- PARSER -----
class AnalizadorSintactico {
public:
    AnalizadorSintactico(const vector<Token>& tokens) : tokens_(tokens), idx_actual_(0) {}

    map<string, string> parsear() {
        map<string, string> ast;
        while (idx_actual_ < tokens_.size()) {
            // saltar comentarios
            saltarComentarios();
            if (idx_actual_ >= tokens_.size()) break;
            if (peek_token().tipo != IDENTIFICADOR) {
                // saltar tokens inesperados hasta el siguiente identificador
                obtener_token();
                continue;
            }
            string clave = obtener_token().valor;
            // manejo especial para declaraciones 'enum' y 'struct'
            if (clave == "enum") {
                saltarComentarios();
                // siguiente debe ser el nombre del enum
                if (peek_token().tipo != IDENTIFICADOR) throw runtime_error("Error: Se esperaba nombre de enum.");
                string nombreEnum = obtener_token().valor;
                saltarComentarios();
                if (peek_token().tipo != LLAVE_ABIERTA) throw runtime_error("Error: Se esperaba '{' en enum.");
                // parsear cuerpo del enum
                obtener_token(); // consumir '{'
                ostringstream oss;
                oss << "{";
                while (true) {
                    saltarComentarios();
                    if (peek_token().tipo == LLAVE_CERRADA) break;
                    if (peek_token().tipo != IDENTIFICADOR) throw runtime_error("Error en enum: Se esperaba identificador.");
                    string entrada = obtener_token().valor;
                    saltarComentarios();
                    if (peek_token().tipo == DOS_PUNTOS) { obtener_token(); saltarComentarios(); }
                    string val;
                    if (peek_token().tipo == NUMERO) val = obtener_token().valor;
                    else val = "null";
                    oss << entrada << ":" << val << ",";
                    saltarComentarios();
                    if (peek_token().tipo == COMA) { obtener_token(); saltarComentarios(); }
                }
                if (peek_token().tipo == LLAVE_CERRADA) obtener_token();
                oss << "}";
                ast[nombreEnum] = oss.str();
                continue;
            }
            if (clave == "struct") {
                saltarComentarios();
                if (peek_token().tipo != IDENTIFICADOR) throw runtime_error("Error: Se esperaba nombre de struct.");
                string nombreStruct = obtener_token().valor;
                saltarComentarios();
                if (peek_token().tipo != LLAVE_ABIERTA) throw runtime_error("Error: Se esperaba '{' en struct.");
                // parsear cuerpo del struct
                obtener_token(); // consumir '{'
                ostringstream oss;
                oss << "{";
                while (true) {
                    saltarComentarios();
                    if (peek_token().tipo == LLAVE_CERRADA) break;
                    if (peek_token().tipo == IDENTIFICADOR) {
                        string campo = obtener_token().valor;
                        // punto y coma opcional
                        if (peek_token().tipo == PUNTO_Y_COMA) obtener_token();
                        oss << campo << ":null,";
                        saltarComentarios();
                        continue;
                    }
                    // saltar tokens inesperados
                    obtener_token();
                }
                if (peek_token().tipo == LLAVE_CERRADA) obtener_token();
                oss << "}";
                ast[nombreStruct] = oss.str();
                continue;
            }
            saltarComentarios();
            if (peek_token().tipo != IGUAL) {
                throw runtime_error("Error de sintaxis: Se esperaba '=' después de '" + clave + "'.");
            }
            obtener_token(); // consumir '='
            saltarComentarios();
                string val = parsearValor();
            ast[clave] = val;
            saltarComentarios();
        }
        return ast;
    }

private:
    const vector<Token>& tokens_;
    size_t idx_actual_;

    void saltarComentarios() {
        while (idx_actual_ < tokens_.size() && tokens_[idx_actual_].tipo == COMENTARIO) idx_actual_++;
    }

    const Token& peek_token() const {
        size_t p = idx_actual_;
        while (p < tokens_.size() && tokens_[p].tipo == COMENTARIO) p++;
    if (p >= tokens_.size()) throw runtime_error("Error de sintaxis: Fin inesperado del archivo.");
        return tokens_[p];
    }

    const Token& obtener_token() {
    if (idx_actual_ >= tokens_.size()) throw runtime_error("Error de sintaxis: Fin inesperado del archivo.");
        return tokens_[idx_actual_++];
    }

    string parsearValor() {
        const Token& tok = peek_token();
        if (tok.tipo == CADENA) {
            string s = obtener_token().valor;
            // quitar comillas si están presentes
            if (s.size() >= 2 && s.front() == '"' && s.back() == '"') s = s.substr(1, s.size()-2);
            // devolver cadena entre comillas
            return string("\"") + s + string("\"");
        }
        if (tok.tipo == NUMERO || tok.tipo == IDENTIFICADOR) {
            return obtener_token().valor;
        }
        if (tok.tipo == LLAVE_ABIERTA) {
            return parsearBloque();
        }
        if (tok.tipo == CORCHETE_ABIERTO) {
            return parsearLista();
        }
    throw runtime_error("Error de sintaxis: Valor inesperado.");
    }

    string parsearBloque() {
        obtener_token(); // consumir '{'
        map<string, string> bloque;
        saltarComentarios();
        while (peek_token().tipo != LLAVE_CERRADA) {
            if (peek_token().tipo != IDENTIFICADOR) {
                throw runtime_error("Error de sintaxis en bloque: Se esperaba un identificador.");
            }
            string clave = obtener_token().valor;
            saltarComentarios();
            Token sep = peek_token();
            if (sep.tipo == DOS_PUNTOS || sep.tipo == IGUAL) {
                obtener_token(); // consumir ':' o '='
            } else {
                throw runtime_error("Error de sintaxis en bloque: Se esperaba ':' o '=' después de '" + clave + "'.");
            }
            saltarComentarios();
            string val = parsearValor();
            bloque[clave] = val;
            saltarComentarios();
            if (peek_token().tipo == COMA) obtener_token();
            saltarComentarios();
        }
        obtener_token(); // consumir '}'
    ostringstream oss;
        oss << "{";
        for (auto const& kv : bloque) {
            oss << kv.first << ":" << kv.second << ",";
        }
        oss << "}";
        return oss.str();
    }

    string parsearLista() {
        obtener_token(); // consumir '['
        vector<string> elementos;
        saltarComentarios();
        while (peek_token().tipo != CORCHETE_CERRADO) {
            const Token& it = peek_token();
            if (it.tipo == IDENTIFICADOR || it.tipo == NUMERO) {
                elementos.push_back(obtener_token().valor);
            } else if (it.tipo == CADENA) {
                string s = obtener_token().valor;
                if (s.size() >= 2 && s.front() == '"' && s.back() == '"') s = s.substr(1, s.size()-2);
                elementos.push_back(string("\"") + s + string("\"") );
            } else if (it.tipo == CORCHETE_ABIERTO) {
                elementos.push_back(parsearLista());
            } else if (it.tipo == LLAVE_ABIERTA) {
                elementos.push_back(parsearBloque());
            } else {
                throw runtime_error("Error de sintaxis en lista: Se esperaba un valor.");
            }
            saltarComentarios();
            if (peek_token().tipo == COMA) obtener_token();
            saltarComentarios();
        }
        obtener_token(); // consumir ']'
    ostringstream oss;
        oss << "[";
        for (const auto& it : elementos) oss << it << ",";
        oss << "]";
        return oss.str();
    }
};

// Pretty print for AST produced by the new Parser (map<string,string>)
string indentStr(int n) { return string(n*2, ' '); }
void printAstMap(const map<string, string>& m, ostream& out, int indent=0) {
    out << "{" << endl;
    for (auto it = m.begin(); it != m.end(); ++it) {
        out << indentStr(indent+1) << '"' << it->first << '"' << ": " << it->second;
        if (next(it) != m.end()) out << ",";
        out << endl;
    }
    out << indentStr(indent) << "}";
}

int main(int argc, char** argv) {
    try {
        string nombreArchivo = "Tetris.brik";
        if (argc >= 2) nombreArchivo = argv[1];

        string contenido = cargarArchivo(nombreArchivo);
        AnalizadorLexico lexer(contenido);
        vector<Token> tokens = lexer.tokenizar();

        cout << "\n=== RESUMEN ===" << endl;
        cout << "Total de tokens reconocidos: " << tokens.size() << endl;
        cout << "\nTipos de tokens encontrados:" << endl;
        
        map<string, int> cuenta_tokens;
        for (const Token& token : tokens) {
            string nombre_token;
            switch (token.tipo) {
                case IDENTIFICADOR: nombre_token = "IDENTIFICADOR"; break;
                case CADENA: nombre_token = "CADENA"; break;
                case NUMERO: nombre_token = "NUMERO"; break;
                case OPERADOR: nombre_token = "OPERADOR"; break;
                case LLAVE_ABIERTA: nombre_token = "LLAVE_ABIERTA"; break;
                case LLAVE_CERRADA: nombre_token = "LLAVE_CERRADA"; break;
                case CORCHETE_ABIERTO: nombre_token = "CORCHETE_ABIERTO"; break;
                case CORCHETE_CERRADO: nombre_token = "CORCHETE_CERRADO"; break;
                case COMA: nombre_token = "COMA"; break;
                case IGUAL: nombre_token = "IGUAL"; break;
                case DOS_PUNTOS: nombre_token = "DOS_PUNTOS"; break;
                case PUNTO_Y_COMA: nombre_token = "PUNTO_Y_COMA"; break;
                case COMENTARIO: nombre_token = "COMENTARIO"; break;
            }
            cuenta_tokens[nombre_token]++;
        }

        for (const auto& pair : cuenta_tokens) {
            cout << "  " << pair.first << ": " << pair.second << " tokens" << endl;
        }

        cout << "\n=== PRIMEROS 30 TOKENS ===" << endl;
        int contador = 0;
        for (const Token& token : tokens) {
            if (contador >= 30) break;
            string nombre_token;
            switch (token.tipo) {
                case IDENTIFICADOR: nombre_token = "IDENTIFICADOR"; break;
                case CADENA: nombre_token = "CADENA"; break;
                case NUMERO: nombre_token = "NUMERO"; break;
                case OPERADOR: nombre_token = "OPERADOR"; break;
                case LLAVE_ABIERTA: nombre_token = "LLAVE_ABIERTA"; break;
                case LLAVE_CERRADA: nombre_token = "LLAVE_CERRADA"; break;
                case CORCHETE_ABIERTO: nombre_token = "CORCHETE_ABIERTO"; break;
                case CORCHETE_CERRADO: nombre_token = "CORCHETE_CERRADO"; break;
                case COMA: nombre_token = "COMA"; break;
                case IGUAL: nombre_token = "IGUAL"; break;
                case DOS_PUNTOS: nombre_token = "DOS_PUNTOS"; break;
                case PUNTO_Y_COMA: nombre_token = "PUNTO_Y_COMA"; break;
                case COMENTARIO: nombre_token = "COMENTARIO"; break;
            }
            cout << nombre_token << "(\"" << token.valor << "\")" << endl;
            contador++;
        }

        // Parsear
        AnalizadorSintactico analizador(tokens);
        map<string, string> ast = analizador.parsear();

        cout << "\n=== ESTRUCTURA PARSEADA ===" << endl;
        printAstMap(ast, cout, 0);
        cout << endl;

        // Escribir arbol.ast
        ofstream out("arbol.ast");
        if (!out.is_open()) {
            cerr << "No se pudo crear arbol.ast" << endl;
            return 1;
        }
        printAstMap(ast, out, 0);
        out << endl;
        out.close();

        cout << "\nAST guardado en arbol.ast" << endl;

    } catch (const runtime_error& e) {
        cerr << e.what() << endl;
        return 1;
    }
}

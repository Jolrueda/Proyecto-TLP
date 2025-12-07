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

string cargarArchivo(const string& ruta) {
    ifstream archivo(ruta.c_str());
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
                    Token tcom;
                    tcom.tipo = COMENTARIO;
                    tcom.valor = comentario;
                    tokens.push_back(tcom);
                    cout << "TOKEN COMENTARIO: " << comentario << endl;
                    continue;
                }
                else {
                    Token toper;
                    toper.tipo = OPERADOR;
                    toper.valor = "/";
                    tokens.push_back(toper);
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
                
                Token tstr;
                tstr.tipo = CADENA;
                tstr.valor = valor_cadena;
                tokens.push_back(tstr);
                cout << "TOKEN CADENA: " << valor_cadena << endl;
                continue;
            }

            // Números (enteros y decimales, incluyendo negativos)
            // Verificar si es un número (puede empezar con '-' seguido de dígito)
            bool es_numero = isdigit(caracter_actual);
            bool es_negativo = false;
            if (caracter_actual == '-' && pos_actual_ + 1 < fuente_.length() && isdigit(fuente_[pos_actual_ + 1])) {
                es_numero = true;
                es_negativo = true;
            }
            
            if (es_numero) {
                string numero;
                if (es_negativo) {
                    numero += '-';
                    pos_actual_++; // Avanzar después del '-'
                }
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
                
                Token tnum;
                tnum.tipo = NUMERO;
                tnum.valor = numero;
                tokens.push_back(tnum);
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
                Token tid;
                tid.tipo = IDENTIFICADOR;
                tid.valor = identificador;
                tokens.push_back(tid);
                cout << "TOKEN IDENTIFICADOR: " << identificador << endl;
                continue;
            }

            // Operador de asignación (=)
            if (caracter_actual == '=') {
                Token tig;
                tig.tipo = IGUAL;
                tig.valor = "=";
                tokens.push_back(tig);
                cout << "TOKEN IGUAL: =" << endl;
                pos_actual_++;
                continue;
            }

            // Dos puntos (:)
            if (caracter_actual == ':') {
                Token td;
                td.tipo = DOS_PUNTOS;
                td.valor = ":";
                tokens.push_back(td);
                cout << "TOKEN DOS_PUNTOS: :" << endl;
                pos_actual_++;
                continue;
            }

            // Punto y coma (;)
            if (caracter_actual == ';') {
                Token tp;
                tp.tipo = PUNTO_Y_COMA;
                tp.valor = ";";
                tokens.push_back(tp);
                cout << "TOKEN PUNTO_Y_COMA: ;" << endl;
                pos_actual_++;
                continue;
            }

            // Llaves
            if (caracter_actual == '{') {
                Token tla;
                tla.tipo = LLAVE_ABIERTA;
                tla.valor = "{";
                tokens.push_back(tla);
                cout << "TOKEN LLAVE_ABIERTA: {" << endl;
                pos_actual_++;
                continue;
            }

            if (caracter_actual == '}') {
                Token tlc;
                tlc.tipo = LLAVE_CERRADA;
                tlc.valor = "}";
                tokens.push_back(tlc);
                cout << "TOKEN LLAVE_CERRADA: }" << endl;
                pos_actual_++;
                continue;
            }

            // Corchetes
            if (caracter_actual == '[') {
                Token tca;
                tca.tipo = CORCHETE_ABIERTO;
                tca.valor = "[";
                tokens.push_back(tca);
                cout << "TOKEN CORCHETE_ABIERTO: [" << endl;
                pos_actual_++;
                continue;
            }

            if (caracter_actual == ']') {
                Token tcc;
                tcc.tipo = CORCHETE_CERRADO;
                tcc.valor = "]";
                tokens.push_back(tcc);
                cout << "TOKEN CORCHETE_CERRADO: ]" << endl;
                pos_actual_++;
                continue;
            }

            // Coma
            if (caracter_actual == ',') {
                Token tcoma;
                tcoma.tipo = COMA;
                tcoma.valor = ",";
                tokens.push_back(tcoma);
                cout << "TOKEN COMA: ," << endl;
                pos_actual_++;
                continue;
            }

            // Otros operadores básicos (excluyendo '-' que ya se maneja para números negativos)
            if (caracter_actual == '+' || caracter_actual == '*' || 
                caracter_actual == '%' || caracter_actual == '!') {
                Token top;
                top.tipo = OPERADOR;
                top.valor = string(1, caracter_actual);
                tokens.push_back(top);
                cout << "TOKEN OPERADOR: " << caracter_actual << endl;
                pos_actual_++;
                continue;
            }
            
            // Operador '-' solo si no es parte de un número negativo
            if (caracter_actual == '-') {
                // Verificar si el siguiente carácter es un dígito
                if (pos_actual_ + 1 < fuente_.length() && isdigit(fuente_[pos_actual_ + 1])) {
                    // Es parte de un número negativo, se manejará en la sección de números
                    // No hacer nada aquí, dejar que el siguiente ciclo lo maneje
                } else {
                    // Es un operador de resta
                    Token top;
                    top.tipo = OPERADOR;
                    top.valor = "-";
                    tokens.push_back(top);
                    cout << "TOKEN OPERADOR: -" << endl;
                    pos_actual_++;
                    continue;
                }
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
        // Saltar comentarios antes de obtener el token
        while (idx_actual_ < tokens_.size() && tokens_[idx_actual_].tipo == COMENTARIO) idx_actual_++;
        if (idx_actual_ >= tokens_.size()) throw runtime_error("Error de sintaxis: Fin inesperado del archivo.");
        return tokens_[idx_actual_++];
    }

    string parsearValor() {
        const Token& tok = peek_token();
        if (tok.tipo == CADENA) {
            string s = obtener_token().valor;
            // quitar comillas si están presentes
            if (s.size() >= 2 && s[0] == '"' && s[s.size()-1] == '"') s = s.substr(1, s.size()-2);
            // devolver cadena entre comillas
            return string("\"") + s + string("\"");
        }
        if (tok.tipo == NUMERO) {
            return obtener_token().valor;
        }
        if (tok.tipo == IDENTIFICADOR) {
            string valor = obtener_token().valor;
            // Verificar si es un valor booleano
            if (valor == "true" || valor == "false") {
                return valor;  // Devolver el valor booleano tal cual
            }
            // Si no es booleano, devolver como identificador
            return valor;
        }
        if (tok.tipo == LLAVE_ABIERTA) {
            return parsearBloque();
        }
        if (tok.tipo == CORCHETE_ABIERTO) {
            return parsearLista();
        }
        // Si llegamos aquí, el token no es un tipo válido para un valor
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
        for (map<string, string>::const_iterator it = bloque.begin(); it != bloque.end(); ++it) {
            oss << it->first << ":" << it->second << ",";
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
                if (s.size() >= 2 && s[0] == '"' && s[s.size()-1] == '"') s = s.substr(1, s.size()-2);
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
        for (size_t i = 0; i < elementos.size(); ++i) oss << elementos[i] << ",";
        oss << "]";
        return oss.str();
    }
};

// Pretty print for AST produced by the new Parser (map<string,string>)
string indentStr(int n) { return string(n*2, ' '); }
void printAstMap(const map<string, string>& m, ostream& out, int indent=0) {
    out << "{" << endl;
    for (map<string, string>::const_iterator it = m.begin(); it != m.end(); ++it) {
        out << indentStr(indent+1) << '"' << it->first << '"' << ": " << it->second;
        map<string, string>::const_iterator next_it = it;
        ++next_it;
        if (next_it != m.end()) out << ",";
        out << endl;
    }
    out << indentStr(indent) << "}";
}

int main(int argc, char** argv) {
    try {
        string nombreArchivo;
        
        // Si se pasa argumento, usar ese archivo
        if (argc >= 2) {
            // Soportar atajos por nombre de juego: "tetris" o "snake"
            string arg = argv[1];
            // normalizar a minuscula
            for (size_t _i = 0; _i < arg.size(); ++_i) {
                arg[_i] = (char)tolower((unsigned char)arg[_i]);
            }
            if (arg == "tetris" || arg == "t") {
                nombreArchivo = "config/games/Tetris.brik";
                cout << "Compilando Tetris.brik..." << endl;
            } else if (arg == "snake" || arg == "s") {
                nombreArchivo = "config/games/Snake.brik";
                cout << "Compilando Snake.brik..." << endl;
            } else {
                // Se asume que es una ruta a archivo .brik
                nombreArchivo = argv[1];
            }
        }
        // Si no, mostrar menú de selección
        else {
            cout << "============================================" << endl;
            cout << "    COMPILADOR MOTOR DE LADRILLOS" << endl;
            cout << "============================================" << endl;
            cout << "Selecciona el juego a compilar:" << endl;
            cout << "  1. Tetris.brik" << endl;
            cout << "  2. Snake.brik" << endl;
            cout << "  3. Archivo personalizado" << endl;
            cout << "Opcion (1-3): ";
            
            int opcion;
            cin >> opcion;
            
            switch(opcion) {
                case 1:
                    nombreArchivo = "config/games/Tetris.brik";
                    cout << "Compilando Tetris.brik..." << endl;
                    break;
                case 2:
                    nombreArchivo = "config/games/Snake.brik";
                    cout << "Compilando Snake.brik..." << endl;
                    break;
                case 3:
                    cout << "Ingresa la ruta del archivo .brik: ";
                    cin >> nombreArchivo;
                    break;
                default:
                    cout << "Opcion invalida. Usando Tetris.brik por defecto." << endl;
                    nombreArchivo = "config/games/Tetris.brik";
                    break;
            }
            cout << endl;
        }

        string contenido = cargarArchivo(nombreArchivo);
        
        AnalizadorLexico lexer(contenido);
        vector<Token> tokens = lexer.tokenizar();

        cout << "\n=== RESUMEN ===" << endl;
        cout << "Total de tokens reconocidos: " << tokens.size() << endl;
        cout << "\nTipos de tokens encontrados:" << endl;
        
        map<string, int> cuenta_tokens;
        for (size_t i = 0; i < tokens.size(); ++i) {
            const Token& token = tokens[i];
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

        for (map<string, int>::const_iterator it = cuenta_tokens.begin(); it != cuenta_tokens.end(); ++it) {
            cout << "  " << it->first << ": " << it->second << " tokens" << endl;
        }

        cout << "\n=== PRIMEROS 30 TOKENS ===" << endl;
        int contador = 0;
        for (size_t i = 0; i < tokens.size() && contador < 30; ++i) {
            const Token& token = tokens[i];
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
        ofstream out("build/arbol.ast");
        if (!out.is_open()) {
            cerr << "No se pudo crear build/arbol.ast" << endl;
            return 1;
        }
        printAstMap(ast, out, 0);
        out << endl;
        out.close();

        cout << "\nAST guardado en build/arbol.ast" << endl;
        cout << "Compilacion completada para: " << nombreArchivo << endl;

    } catch (const runtime_error& e) {
        cerr << e.what() << endl;
        return 1;
    }
    return 0;
}

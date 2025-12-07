// ============================================================================
// RUNTIME UNIFICADO - MOTOR DE LADRILLOS
// ============================================================================
// Este archivo contiene el runtime principal que permite ejecutar juegos
// de tipo "ladrillos" como Tetris y Snake en un solo binario.
//
// Características:
//   - Parser AST simple para cargar configuraciones desde archivos .brik
//   - Motor de Tetris con física completa y sistema de niveles
//   - Motor de Snake con crecimiento y colisiones
//   - Renderizado optimizado sin parpadeo
//   - Soporte para modo consola (texto) y modo gráfico (ventana GDI)
//   - Compatible con Windows XP (usa GetTickCount en lugar de <chrono>)
//
// Compilación:
//   Modo consola:     g++ -o runtime runtime.cpp
//   Modo gráfico GDI: g++ -DUSE_GDI -o runtime runtime.cpp -lgdi32 -luser32
//
// ============================================================================

#include <algorithm>
#include <ctime>
#include <conio.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

using namespace std;

// ============================================================================
// CONFIGURACIÓN Y CONSTANTES GLOBALES
// ============================================================================

// Modo de renderizado disponible
enum ModoRenderizado { 
    MODO_CONSOLA = 1,  // Renderizado en consola con caracteres ASCII
    MODO_VENTANA = 2   // Renderizado en ventana gráfica (requiere USE_GDI)
};

// Variable global que almacena el modo de renderizado seleccionado
ModoRenderizado modoActual = MODO_CONSOLA;


// ============================================================================
// CLASE: ColorConsola
// ============================================================================
// Gestiona los colores de texto en la consola de Windows.
// Guarda el color original y lo restaura automáticamente al destruirse.
// ============================================================================
class ColorConsola {
private:
    HANDLE hConsole;      // Handle de la consola de Windows
    WORD   color_original; // Color original que se restaurará al destruirse

public:
    /**
     * Constructor: obtiene el handle de la consola y guarda el color actual.
     */
    ColorConsola() {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(hConsole, &info);
        color_original = info.wAttributes;
    }
    
    /**
     * Destructor: restaura el color original de la consola.
     */
    ~ColorConsola() { 
        restaurarColor(); 
    }

    /**
     * Establece el color del texto en la consola.
     * @param codigo_color Código de color de Windows (WORD)
     */
    void establecerColor(int codigo_color) {
        SetConsoleTextAttribute(hConsole, codigo_color);
    }

    /**
     * Restaura el color original de la consola.
     */
    void restaurarColor() {
        SetConsoleTextAttribute(hConsole, color_original);
    }

   
    string obtenerColorAnsi(const string& nombre_color) {
        if (nombre_color == "verde_claro" || nombre_color == "verde") {
            return "\033[92m";
        }
        if (nombre_color == "verde_oscuro") {
            return "\033[32m";
        }
        if (nombre_color == "verde_medio") {
            return "\033[36m";
        }
        if (nombre_color == "rojo") {
            return "\033[91m";
        }
        if (nombre_color == "amarillo") {
            return "\033[93m";
        }
        if (nombre_color == "blanco") {
            return "\033[97m";
        }
        if (nombre_color == "gris") {
            return "\033[90m";
        }
        if (nombre_color == "negro") {
            return "\033[30m";
        }
        return "\033[37m";  // Blanco por defecto
    }
};

// ============================================================================
// SINGLETON PARA GESTIÓN DE COLORES EN CONSOLA
// ============================================================================
// Proporciona una única instancia global de ColorConsola para evitar
// múltiples inicializaciones del handle de consola.
ColorConsola& obtenerColorConsola() {
    static ColorConsola instancia;
    return instancia;
}

// ================================================================
// AST parsers simples (cada juego usa el suyo)
// ================================================================
// --- Parser para Tetris ---
class ASTParser {
public:
    map<string, string>                 strings;
    map<string, int>                    integers;
    map<string, map<string, int> >       objects_int;
    map<string, map<string, string> >    objects_str;

    bool cargarDesdeAST(const string& archivo) {
        ifstream file(archivo.c_str());
        if (!file.is_open()) {
            return false;
        }
        string linea;
        while (getline(file, linea)) {
            procesarLinea(linea);
        }
        return true;
    }
    int obtenerInt(const string& clave, int defecto = 0) {
        map<string,int>::iterator it = integers.find(clave);
        return (it != integers.end()) ? it->second : defecto;
    }

    string obtenerString(const string& clave, const string& defecto = "") {
        map<string,string>::iterator it = strings.find(clave);
        return (it != strings.end()) ? it->second : defecto;
    }
private:
    void procesarLinea(const string& linea) {
        if (linea.find("\"nombre_juego\":") != string::npos) {
            strings["nombre_juego"] = extraerString(linea);
        } else if (linea.find("\"ancho_tablero\":") != string::npos) {
            integers["ancho_tablero"] = extraerInt(linea);
        } else if (linea.find("\"alto_tablero\":") != string::npos) {
            integers["alto_tablero"] = extraerInt(linea);
        } else if (linea.find("\"velocidad_inicial\":") != string::npos) {
            integers["velocidad_inicial"] = extraerInt(linea);
        } else if (linea.find("\"velocidad_caida_rapida\":") != string::npos) {
            integers["velocidad_caida_rapida"] = extraerInt(linea);
        } else if (linea.find("\"tiempo_fijacion_pieza\":") != string::npos) {
            integers["tiempo_fijacion_pieza"] = extraerInt(linea);
        } else if (linea.find("\"tiempo_antes_de_bloquear\":") != string::npos) {
            integers["tiempo_antes_de_bloquear"] = extraerInt(linea);
        } else if (linea.find("\"aceleracion_por_nivel\":") != string::npos) {
            integers["aceleracion_por_nivel"] = extraerInt(linea);
        } else if (linea.find("\"velocidad_maxima\":") != string::npos) {
            integers["velocidad_maxima"] = extraerInt(linea);
        } else if (linea.find("\"gravedad_automatica\":") != string::npos) {
            // boolean-like
            integers["gravedad_automatica"] = (linea.find("true") != string::npos) ? 1 : 0;
        } else if (linea.find("\"lineas_para_nivel\":") != string::npos) {
            integers["lineas_para_nivel"] = extraerInt(linea);
        } else if (linea.find("\"tamanio_celda\":") != string::npos) {
            integers["tamanio_celda"] = extraerInt(linea);
        } else if (linea.find("\"nivel_inicial\":") != string::npos) {
            integers["nivel_inicial"] = extraerInt(linea);
        } else if (linea.find("\"puntos_linea_simple\":") != string::npos) {
            integers["puntos_linea_simple"] = extraerInt(linea);
        } else if (linea.find("\"puntos_linea_doble\":") != string::npos) {
            integers["puntos_linea_doble"] = extraerInt(linea);
        } else if (linea.find("\"puntos_linea_triple\":") != string::npos) {
            integers["puntos_linea_triple"] = extraerInt(linea);
        } else if (linea.find("\"puntos_linea_tetris\":") != string::npos) {
            integers["puntos_linea_tetris"] = extraerInt(linea);
        } else if (linea.find("\"codigos_color\":") != string::npos) {
            parsearColores();
        } else if (linea.find("\"colores_piezas\":") != string::npos) {
            parsearColoresPiezas();
        }
    }
    string extraerString(const string& linea) {
        size_t inicio = linea.find("\"", linea.find(":")) + 1;
        size_t fin    = linea.find("\"", inicio);
        return (inicio != string::npos && fin != string::npos)
            ? linea.substr(inicio, fin - inicio)
            : "";
    }
    int extraerInt(const string& linea) {
        size_t inicio = linea.find(":") + 1;
        size_t fin    = linea.find(",", inicio);
        if (fin == string::npos) fin = linea.length();
        string num_str = linea.substr(inicio, fin - inicio);
        num_str.erase(0, num_str.find_first_not_of(" \t"));
        num_str.erase(num_str.find_last_not_of(" \t,") + 1);
        return atoi(num_str.c_str());
    }
    void parsearColores() {
        objects_int["codigos_color"]["amarillo"] = 14;
        objects_int["codigos_color"]["azul"] = 9;
        objects_int["codigos_color"]["blanco"] = 15;
        objects_int["codigos_color"]["cian"] = 11;
        objects_int["codigos_color"]["gris"] = 8;
        objects_int["codigos_color"]["magenta"] = 13;
        objects_int["codigos_color"]["naranja"] = 12;
        objects_int["codigos_color"]["rojo"] = 12;
        objects_int["codigos_color"]["verde"] = 10;
    }
    void parsearColoresPiezas() {
        objects_str["colores_piezas"]["I"] = "cian";
        objects_str["colores_piezas"]["J"] = "azul";
        objects_str["colores_piezas"]["L"] = "naranja";
        objects_str["colores_piezas"]["O"] = "amarillo";
        objects_str["colores_piezas"]["S"] = "verde";
        objects_str["colores_piezas"]["T"] = "magenta";
        objects_str["colores_piezas"]["Z"] = "rojo";
    }
};

// --- Parser para Snake ---
class SnakeAST {
public:
    map<string, string>         strings;
    map<string, int>            integers;
    map<string, bool>           booleans;
    map<string, vector<string> > arrays;

    bool cargarDesdeAST(const string& archivo) {
        ifstream file(archivo.c_str());
        if (!file.is_open()) {
            return false;
        }
        string linea;
        while (getline(file, linea)) {
            procesarLinea(linea);
        }
        return true;
    }
private:
    void procesarLinea(const string& linea) {
        if (linea.find("\"nombre_juego\":") != string::npos) {
            strings["nombre_juego"] = extraerString(linea);
        } else if (linea.find("\"ancho_tablero\":") != string::npos) {
            integers["ancho_tablero"] = extraerInt(linea);
        } else if (linea.find("\"alto_tablero\":") != string::npos) {
            integers["alto_tablero"] = extraerInt(linea);
        } else if (linea.find("\"velocidad_inicial\":") != string::npos) {
            integers["velocidad_inicial"] = extraerInt(linea);
        } else if (linea.find("\"puntos_por_movimiento\":") != string::npos) {
            integers["puntos_por_movimiento"] = extraerInt(linea);
        } else if (linea.find("\"puntos_por_fruta\":") != string::npos) {
            integers["puntos_por_fruta"] = extraerInt(linea);
        } else if (linea.find("\"longitud_inicial\":") != string::npos) {
            integers["longitud_inicial"] = extraerInt(linea);
        } else if (linea.find("\"terminar_al_chocar_borde\":") != string::npos) {
            booleans["terminar_al_chocar_borde"] = (linea.find("true") != string::npos);
        } else if (linea.find("\"terminar_al_chocar_cuerpo\":") != string::npos) {
            booleans["terminar_al_chocar_cuerpo"] = (linea.find("true") != string::npos);
        } else if (linea.find("\"colores_snake\":") != string::npos) {
            arrays["colores_snake"] = extraerArray(linea);
        } else if (linea.find("\"color_fruta\":") != string::npos) {
            strings["color_fruta"] = extraerString(linea);
        } else if (linea.find("\"frutas_disponibles\":") != string::npos) {
            arrays["frutas_disponibles"] = extraerArray(linea);
        } else if (linea.find("\"puntos_") != string::npos && linea.find(":") != string::npos) {
            // Capturar dinámicamente puntos_manzana, puntos_cereza, etc.
            size_t inicio_clave = linea.find("\"puntos_") + 1;
            size_t fin_clave = linea.find("\"", inicio_clave + 7);
            if (inicio_clave != string::npos && fin_clave != string::npos) {
                string clave = linea.substr(inicio_clave, fin_clave - inicio_clave);
                integers[clave] = extraerInt(linea);
            }
        } else if (linea.find("\"color_") != string::npos && linea.find(":") != string::npos && linea.find("color_fruta") == string::npos) {
            // Capturar dinámicamente color_manzana, color_cereza, etc. (excluyendo color_fruta)
            size_t inicio_clave = linea.find("\"color_") + 1;
            size_t fin_clave = linea.find("\"", inicio_clave + 6);
            if (inicio_clave != string::npos && fin_clave != string::npos) {
                string clave = linea.substr(inicio_clave, fin_clave - inicio_clave);
                strings[clave] = extraerString(linea);
            }
        } else if (linea.find("\"mensaje_inicio\":") != string::npos) {
            strings["mensaje_inicio"] = extraerString(linea);
        } else if (linea.find("\"mensaje_game_over\":") != string::npos) {
            strings["mensaje_game_over"] = extraerString(linea);
        } else if (linea.find("\"mensaje_pausa\":") != string::npos) {
            strings["mensaje_pausa"] = extraerString(linea);
        }
    }
    string extraerString(const string& linea) {
        size_t inicio = linea.find("\"", linea.find(":")) + 1;
        size_t fin    = linea.find("\"", inicio);
        return (inicio != string::npos && fin != string::npos)
            ? linea.substr(inicio, fin - inicio)
            : "";
    }
    int extraerInt(const string& linea) {
        size_t inicio = linea.find(":") + 1;
        size_t fin    = linea.find(",", inicio);
        if (fin == string::npos) fin = linea.length();
        string num_str = linea.substr(inicio, fin - inicio);
        num_str.erase(0, num_str.find_first_not_of(" \t"));
        num_str.erase(num_str.find_last_not_of(" \t,") + 1);
        return atoi(num_str.c_str());
    }
    vector<string> extraerArray(const string& linea) {
        vector<string> resultado;
        size_t inicio = linea.find("[");
        size_t fin    = linea.find("]");
        if (inicio != string::npos && fin != string::npos) {
            string contenido = linea.substr(inicio + 1, fin - inicio - 1);
            stringstream ss(contenido);
            string item;
            while (getline(ss, item, ',')) {
                size_t a = item.find("\"");
                size_t b = item.find("\"", a + 1);
                if (a != string::npos && b != string::npos) {
                    resultado.push_back(item.substr(a + 1, b - a - 1));
                }
            }
        }
        return resultado;
    }
};

// ================================================================
// Tetris
// ================================================================
class ConfigTetris {
public:
    map<string, int> colores;  // Colores para consola (códigos)
    map<string, vector<int> > colores_rgb;  // Colores RGB para renderizado gráfico [R, G, B]
    map<string, string> pieza_a_color;  // Mapeo de tipo de pieza a nombre de color (ej: "I" -> "cian")
    map<string, vector<vector<vector<int> > > > rotaciones_piezas;
    vector<string> tipos_piezas;
    string nombre_juego;
    int ancho_tablero;
    int alto_tablero;
    int velocidad_inicial;
    // Physics and gameplay params read from AST
    int velocidad_caida_rapida;
    int tiempo_fijacion_pieza;
    int tiempo_antes_de_bloquear;
    int aceleracion_por_nivel;
    int velocidad_maxima;
    bool gravedad_automatica;
    int lineas_para_nivel;
    int tamanio_celda;
    int nivel_inicial;
    int puntos_linea_simple;
    int puntos_linea_doble;
    int puntos_linea_triple;
    int puntos_linea_tetris;

    ConfigTetris() {
        cargarDesdeAST();
        configurarRotacionesHardcoded();
    }
public:
    void printConfig() const {
        std::cout << "[ConfigTetris] nombre_juego=" << nombre_juego
                  << " ancho_tablero=" << ancho_tablero
                  << " alto_tablero=" << alto_tablero
                  << " tamanio_celda=" << tamanio_celda
                  << " velocidad_inicial=" << velocidad_inicial
                  << " aceleracion_por_nivel=" << aceleracion_por_nivel
                  << " lineas_para_nivel=" << lineas_para_nivel
                  << " nivel_inicial=" << nivel_inicial
                  << " puntos_linea_simple=" << puntos_linea_simple
                  << " puntos_linea_tetris=" << puntos_linea_tetris
                  << std::endl;
    }
    
    // Obtener color RGB de una pieza por su tipo
    vector<int> obtenerColorRGB(const string& tipo_pieza) const {
        if (pieza_a_color.count(tipo_pieza)) {
            string color_nombre = pieza_a_color.find(tipo_pieza)->second;
            return obtenerColorRGBPorNombre(color_nombre);
        }
        // Valores por defecto basados en el tipo de pieza
        vector<int> rgb;
        if (tipo_pieza == "I") { rgb.push_back(0); rgb.push_back(255); rgb.push_back(255); } // cian
        else if (tipo_pieza == "J") { rgb.push_back(0); rgb.push_back(100); rgb.push_back(255); } // azul
        else if (tipo_pieza == "L") { rgb.push_back(255); rgb.push_back(165); rgb.push_back(0); } // naranja
        else if (tipo_pieza == "O") { rgb.push_back(255); rgb.push_back(255); rgb.push_back(0); } // amarillo
        else if (tipo_pieza == "S") { rgb.push_back(0); rgb.push_back(255); rgb.push_back(0); } // verde
        else if (tipo_pieza == "Z") { rgb.push_back(255); rgb.push_back(0); rgb.push_back(0); } // rojo
        else if (tipo_pieza == "T") { rgb.push_back(255); rgb.push_back(0); rgb.push_back(255); } // magenta
        else { rgb.push_back(128); rgb.push_back(128); rgb.push_back(128); } // gris
        return rgb;
    }
    
    // Obtener color RGB por nombre de color
    vector<int> obtenerColorRGBPorNombre(const string& nombre_color) const {
        if (colores_rgb.count(nombre_color)) {
            return colores_rgb.find(nombre_color)->second;
        }
        // Valores por defecto
        vector<int> rgb;
        if (nombre_color == "cian") { rgb.push_back(0); rgb.push_back(255); rgb.push_back(255); }
        else if (nombre_color == "azul") { rgb.push_back(0); rgb.push_back(100); rgb.push_back(255); }
        else if (nombre_color == "naranja") { rgb.push_back(255); rgb.push_back(165); rgb.push_back(0); }
        else if (nombre_color == "amarillo") { rgb.push_back(255); rgb.push_back(255); rgb.push_back(0); }
        else if (nombre_color == "verde") { rgb.push_back(0); rgb.push_back(255); rgb.push_back(0); }
        else if (nombre_color == "rojo") { rgb.push_back(255); rgb.push_back(0); rgb.push_back(0); }
        else if (nombre_color == "magenta") { rgb.push_back(255); rgb.push_back(0); rgb.push_back(255); }
        else if (nombre_color == "blanco") { rgb.push_back(255); rgb.push_back(255); rgb.push_back(255); }
        else { rgb.push_back(128); rgb.push_back(128); rgb.push_back(128); }
        return rgb;
    }
private:
    void cargarDesdeAST() {
        ASTParser parser;
        if (!parser.cargarDesdeAST("build/arbol.ast")) {
            nombre_juego = "Tetris Clásico";
            ancho_tablero = 10;
            alto_tablero = 20;
            velocidad_inicial = 800;
            // defaults
            velocidad_caida_rapida = 50;
            tiempo_fijacion_pieza = 1000;
            tiempo_antes_de_bloquear = 1000;
            aceleracion_por_nivel = 50;
            velocidad_maxima = 1000;
            gravedad_automatica = true;
            lineas_para_nivel = 10;
            tamanio_celda = 30;
            nivel_inicial = 1;
            puntos_linea_simple = 100;
            puntos_linea_doble = 300;
            puntos_linea_triple = 500;
            puntos_linea_tetris = 800;
            return;
        }
        nombre_juego = parser.obtenerString("nombre_juego", "Tetris Clásico");
        ancho_tablero = parser.obtenerInt("ancho_tablero", 10);
        alto_tablero = parser.obtenerInt("alto_tablero", 20);
        velocidad_inicial = parser.obtenerInt("velocidad_inicial", 800);
        // Read additional physics/gameplay params
        velocidad_caida_rapida = parser.obtenerInt("velocidad_caida_rapida", 50);
        tiempo_fijacion_pieza = parser.obtenerInt("tiempo_fijacion_pieza", 1000);
        tiempo_antes_de_bloquear = parser.obtenerInt("tiempo_antes_de_bloquear", 1000);
        aceleracion_por_nivel = parser.obtenerInt("aceleracion_por_nivel", 50);
        velocidad_maxima = parser.obtenerInt("velocidad_maxima", 1000);
        gravedad_automatica = (parser.obtenerInt("gravedad_automatica", 1) != 0);
        lineas_para_nivel = parser.obtenerInt("lineas_para_nivel", 10);
        tamanio_celda = parser.obtenerInt("tamanio_celda", 30);
        nivel_inicial = parser.obtenerInt("nivel_inicial", 1);
        puntos_linea_simple = parser.obtenerInt("puntos_linea_simple", 100);
        puntos_linea_doble = parser.obtenerInt("puntos_linea_doble", 300);
        puntos_linea_triple = parser.obtenerInt("puntos_linea_triple", 500);
        puntos_linea_tetris = parser.obtenerInt("puntos_linea_tetris", 800);
        {
            vector<string> tmp_tipos;
            tmp_tipos.push_back("I"); tmp_tipos.push_back("J"); tmp_tipos.push_back("L");
            tmp_tipos.push_back("O"); tmp_tipos.push_back("S"); tmp_tipos.push_back("Z"); tmp_tipos.push_back("T");
            tipos_piezas = tmp_tipos;
        }
        for (size_t _i = 0; _i < tipos_piezas.size(); ++_i) {
            string pieza = tipos_piezas[_i];
            if (parser.objects_str["colores_piezas"].count(pieza)) {
                string color_nombre = parser.objects_str["colores_piezas"][pieza];
                pieza_a_color[pieza] = color_nombre;  // Guardar mapeo pieza -> color
                if (parser.objects_int["codigos_color"].count(color_nombre)) {
                    colores[pieza] = parser.objects_int["codigos_color"][color_nombre];
                }
                // Cargar colores RGB (valores por defecto basados en el nombre del color)
                vector<int> rgb_default;
                if (color_nombre == "cian") {
                    rgb_default.push_back(0); rgb_default.push_back(255); rgb_default.push_back(255);
                } else if (color_nombre == "azul") {
                    rgb_default.push_back(0); rgb_default.push_back(100); rgb_default.push_back(255);
                } else if (color_nombre == "naranja") {
                    rgb_default.push_back(255); rgb_default.push_back(165); rgb_default.push_back(0);
                } else if (color_nombre == "amarillo") {
                    rgb_default.push_back(255); rgb_default.push_back(255); rgb_default.push_back(0);
                } else if (color_nombre == "verde") {
                    rgb_default.push_back(0); rgb_default.push_back(255); rgb_default.push_back(0);
                } else if (color_nombre == "rojo") {
                    rgb_default.push_back(255); rgb_default.push_back(0); rgb_default.push_back(0);
                } else if (color_nombre == "magenta") {
                    rgb_default.push_back(255); rgb_default.push_back(0); rgb_default.push_back(255);
                } else if (color_nombre == "blanco") {
                    rgb_default.push_back(255); rgb_default.push_back(255); rgb_default.push_back(255);
                } else {
                    rgb_default.push_back(128); rgb_default.push_back(128); rgb_default.push_back(128); // gris por defecto
                }
                colores_rgb[color_nombre] = rgb_default;
            }
        }
    }
    void configurarRotacionesHardcoded() {
        // Construcción explícita de rotaciones (C++03 compatible)
        // I
        {
            vector<vector<vector<int> > > rots;
            vector<vector<int> > shape;
            vector<int> row;
            int a1[] = {0,0,0,0}; int a2[] = {1,1,1,1}; int a3[] = {0,0,0,0}; int a4[] = {0,0,0,0};
            row.assign(a1, a1+4); shape.push_back(row);
            row.assign(a2, a2+4); shape.push_back(row);
            row.assign(a3, a3+4); shape.push_back(row);
            row.assign(a4, a4+4); shape.push_back(row);
            rots.push_back(shape);
            shape.clear();
            int b1[] = {0,1,0,0}; int b2[] = {0,1,0,0}; int b3[] = {0,1,0,0}; int b4[] = {0,1,0,0};
            row.assign(b1, b1+4); shape.push_back(row);
            row.assign(b2, b2+4); shape.push_back(row);
            row.assign(b3, b3+4); shape.push_back(row);
            row.assign(b4, b4+4); shape.push_back(row);
            rots.push_back(shape);
            // repetir primeras rotaciones para completar 4
            rots.push_back(rots[0]);
            rots.push_back(rots[1]);
            rotaciones_piezas["I"] = rots;
        }
        // J
        {
            vector<vector<vector<int> > > rots;
            vector<vector<int> > shape; vector<int> row;
            int j1[] = {1,0,0}; int j2[] = {1,1,1}; int j3[] = {0,0,0};
            row.assign(j1, j1+3); shape.push_back(row);
            row.assign(j2, j2+3); shape.push_back(row);
            row.assign(j3, j3+3); shape.push_back(row);
            rots.push_back(shape);
            shape.clear();
            int j4[] = {0,1,1}; int j5[] = {0,1,0}; int j6[] = {0,1,0};
            row.assign(j4, j4+3); shape.push_back(row);
            row.assign(j5, j5+3); shape.push_back(row);
            row.assign(j6, j6+3); shape.push_back(row);
            rots.push_back(shape);
            shape.clear();
            int j7[] = {0,0,0}; int j8[] = {1,1,1}; int j9[] = {0,0,1};
            row.assign(j7, j7+3); shape.push_back(row);
            row.assign(j8, j8+3); shape.push_back(row);
            row.assign(j9, j9+3); shape.push_back(row);
            rots.push_back(shape);
            shape.clear();
            int j10[] = {0,1,0}; int j11[] = {0,1,0}; int j12[] = {1,1,0};
            row.assign(j10, j10+3); shape.push_back(row);
            row.assign(j11, j11+3); shape.push_back(row);
            row.assign(j12, j12+3); shape.push_back(row);
            rots.push_back(shape);
            rotaciones_piezas["J"] = rots;
        }
        // L
        {
            vector<vector<vector<int> > > rots; vector<vector<int> > shape; vector<int> row;
            int l1[] = {0,0,1}; int l2[] = {1,1,1}; int l3[] = {0,0,0};
            row.assign(l1, l1+3); shape.push_back(row);
            row.assign(l2, l2+3); shape.push_back(row);
            row.assign(l3, l3+3); shape.push_back(row);
            rots.push_back(shape);
            shape.clear();
            int l4[] = {0,1,0}; int l5[] = {0,1,0}; int l6[] = {0,1,1};
            row.assign(l4, l4+3); shape.push_back(row);
            row.assign(l5, l5+3); shape.push_back(row);
            row.assign(l6, l6+3); shape.push_back(row);
            rots.push_back(shape);
            shape.clear();
            int l7[] = {0,0,0}; int l8[] = {1,1,1}; int l9[] = {1,0,0};
            row.assign(l7, l7+3); shape.push_back(row);
            row.assign(l8, l8+3); shape.push_back(row);
            row.assign(l9, l9+3); shape.push_back(row);
            rots.push_back(shape);
            shape.clear();
            int l10[] = {1,1,0}; int l11[] = {0,1,0}; int l12[] = {0,1,0};
            row.assign(l10, l10+3); shape.push_back(row);
            row.assign(l11, l11+3); shape.push_back(row);
            row.assign(l12, l12+3); shape.push_back(row);
            rots.push_back(shape);
            rotaciones_piezas["L"] = rots;
        }
        // O (cuadrado)
        {
            vector<vector<vector<int> > > rots; vector<vector<int> > shape; vector<int> row;
            int o1[] = {1,1}; int o2[] = {1,1};
            row.assign(o1, o1+2); shape.push_back(row);
            row.assign(o2, o2+2); shape.push_back(row);
            rots.push_back(shape);
            rots.push_back(shape);
            rots.push_back(shape);
            rots.push_back(shape);
            rotaciones_piezas["O"] = rots;
        }
        // S
        {
            vector<vector<vector<int> > > rots; vector<vector<int> > shape; vector<int> row;
            int s1[] = {0,1,1}; int s2[] = {1,1,0}; int s3[] = {0,0,0};
            row.assign(s1, s1+3); shape.push_back(row);
            row.assign(s2, s2+3); shape.push_back(row);
            row.assign(s3, s3+3); shape.push_back(row);
            rots.push_back(shape);
            shape.clear();
            int s4[] = {0,1,0}; int s5[] = {0,1,1}; int s6[] = {0,0,1};
            row.assign(s4, s4+3); shape.push_back(row);
            row.assign(s5, s5+3); shape.push_back(row);
            row.assign(s6, s6+3); shape.push_back(row);
            rots.push_back(shape);
            rots.push_back(rots[0]);
            rots.push_back(rots[1]);
            rotaciones_piezas["S"] = rots;
        }
        // Z
        {
            vector<vector<vector<int> > > rots; vector<vector<int> > shape; vector<int> row;
            int z1[] = {1,1,0}; int z2[] = {0,1,1}; int z3[] = {0,0,0};
            row.assign(z1, z1+3); shape.push_back(row);
            row.assign(z2, z2+3); shape.push_back(row);
            row.assign(z3, z3+3); shape.push_back(row);
            rots.push_back(shape);
            shape.clear();
            int z4[] = {0,0,1}; int z5[] = {0,1,1}; int z6[] = {0,1,0};
            row.assign(z4, z4+3); shape.push_back(row);
            row.assign(z5, z5+3); shape.push_back(row);
            row.assign(z6, z6+3); shape.push_back(row);
            rots.push_back(shape);
            rots.push_back(rots[0]);
            rots.push_back(rots[1]);
            rotaciones_piezas["Z"] = rots;
        }
        // T
        {
            vector<vector<vector<int> > > rots; vector<vector<int> > shape; vector<int> row;
            int t1[] = {0,1,0}; int t2[] = {1,1,1}; int t3[] = {0,0,0};
            row.assign(t1, t1+3); shape.push_back(row);
            row.assign(t2, t2+3); shape.push_back(row);
            row.assign(t3, t3+3); shape.push_back(row);
            rots.push_back(shape);
            shape.clear();
            int t4[] = {0,1,0}; int t5[] = {0,1,1}; int t6[] = {0,1,0};
            row.assign(t4, t4+3); shape.push_back(row);
            row.assign(t5, t5+3); shape.push_back(row);
            row.assign(t6, t6+3); shape.push_back(row);
            rots.push_back(shape);
            shape.clear();
            int t7[] = {0,0,0}; int t8[] = {1,1,1}; int t9[] = {0,1,0};
            row.assign(t7, t7+3); shape.push_back(row);
            row.assign(t8, t8+3); shape.push_back(row);
            row.assign(t9, t9+3); shape.push_back(row);
            rots.push_back(shape);
            shape.clear();
            int t10[] = {0,1,0}; int t11[] = {1,1,0}; int t12[] = {0,1,0};
            row.assign(t10, t10+3); shape.push_back(row);
            row.assign(t11, t11+3); shape.push_back(row);
            row.assign(t12, t12+3); shape.push_back(row);
            rots.push_back(shape);
            rotaciones_piezas["T"] = rots;
        }
    }
};

enum TipoPieza { I=0, J=1, L=2, O=3, S=4, Z=5, T=6, VACIO=7 };
enum ColorTetris { CIAN=11, AZUL=9, NARANJA=12, AMARILLO=14, VERDE=10, ROJO=12, MAGENTA=13, BLANCO=15, GRIS=8 };

class PiezaTetris {
public:
    vector<vector<vector<int> > > rotaciones;
    string                      tipo_str;
    TipoPieza                   tipo;
    ColorTetris                 color;
    int                         x;
    int                         y;
    int                         rotacion_actual;

    explicit PiezaTetris(TipoPieza t, const ConfigTetris& config)
        : tipo(t), x(0), y(0), rotacion_actual(0) {
        configurarPieza(config);
    }
private:
    void configurarPieza(const ConfigTetris& config) {
        switch (tipo) {
            case I: tipo_str = "I"; break;
            case J: tipo_str = "J"; break;
            case L: tipo_str = "L"; break;
            case O: tipo_str = "O"; break;
            case S: tipo_str = "S"; break;
            case Z: tipo_str = "Z"; break;
            case T: tipo_str = "T"; break;
            default:  tipo_str = "I"; break;
        }
        {
            map<string, vector<vector<vector<int> > > >::const_iterator it = config.rotaciones_piezas.find(tipo_str);
            if (it != config.rotaciones_piezas.end()) {
                rotaciones = it->second;
            }
        }
        {
            map<string,int>::const_iterator it2 = config.colores.find(tipo_str);
            if (it2 != config.colores.end()) {
                color = static_cast<ColorTetris>(it2->second);
            } else {
                color = BLANCO;
            }
        }
    }
};

class TetrisEngine {
private:
    static const int ANCHO = 10;
    static const int ALTO  = 20;

    vector<vector<TipoPieza> >   tablero;
    vector<vector<ColorTetris> > colores_tablero;

    PiezaTetris* pieza_actual;
    PiezaTetris* siguiente_pieza;

    // usando rand() en lugar de <random>

    int puntos;
    int nivel;
    int lineas_completadas;
    int contador_linea_simple;  // Contador de líneas simples completadas
    int contador_linea_doble;    // Contador de líneas dobles completadas
    int contador_linea_triple;   // Contador de líneas triples completadas
    int contador_linea_tetris;   // Contador de tetris (4 líneas) completados
    int velocidad_caida;
    DWORD last_horiz_move;
    DWORD last_soft_drop;

    bool juego_activo;
    bool pausado;
    bool game_over;

    DWORD ultimo_movimiento;
    DWORD ultima_caida;

    ConfigTetris config;

    string obtenerColorAnsi(int c) {
        switch (c) {
            case 9:  return "\033[94m";  // Azul claro
            case 10: return "\033[92m";  // Verde claro
            case 11: return "\033[96m";  // Cian
            case 12: return "\033[91m";  // Rojo
            case 13: return "\033[95m";  // Magenta
            case 14: return "\033[93m";  // Amarillo
            case 15: return "\033[97m";  // Blanco
            default: return "\033[37m";  // Blanco normal
        }
    }
public:
    TetrisEngine()
        : tablero(ALTO, vector<TipoPieza>(ANCHO, VACIO))
        , colores_tablero(ALTO, vector<ColorTetris>(ANCHO, GRIS))
        , pieza_actual(NULL)
        , siguiente_pieza(NULL)
        , puntos(0)
        , nivel(1)
        , lineas_completadas(0)
        , contador_linea_simple(0)
        , contador_linea_doble(0)
        , contador_linea_triple(0)
        , contador_linea_tetris(0)
        , velocidad_caida(800)
        , juego_activo(true)
        , pausado(false)
        , game_over(false) {
        // Inicializar temporizadores usando GetTickCount (ms)
        DWORD ahora = GetTickCount();
        ultimo_movimiento = ahora;
        ultima_caida      = ahora;
        // Inicializar semilla aleatoria
        srand((unsigned)time(NULL));
        last_horiz_move = GetTickCount();
        last_soft_drop = GetTickCount();
        // Apply config-driven physics
        nivel = (config.nivel_inicial > 0) ? config.nivel_inicial : 1;
        velocidad_caida = config.velocidad_inicial;
        lineas_completadas = 0;
        generarNuevaPieza();
        generarSiguientePieza();
    }

    ~TetrisEngine() {
        delete pieza_actual;
        delete siguiente_pieza;
    }

    void generarNuevaPieza() {
        if (siguiente_pieza) {
            delete pieza_actual;
            pieza_actual   = siguiente_pieza;
            siguiente_pieza = NULL;
        } else {
            pieza_actual = new PiezaTetris(static_cast<TipoPieza>(rand() % 7), config);
        }

        pieza_actual->x = ANCHO / 2 - 2;
        pieza_actual->y = 0;
        pieza_actual->rotacion_actual = 0;

        if (!esMovimientoValido(pieza_actual->x, pieza_actual->y, pieza_actual->rotacion_actual)) {
            game_over    = true;
            juego_activo = false;
        }
    }

    void generarSiguientePieza() {
        siguiente_pieza = new PiezaTetris(static_cast<TipoPieza>(rand() % 7), config);
    }

    bool esMovimientoValido(int nx, int ny, int nr) {
        if (!pieza_actual) return false;

        vector<vector<int> >& forma = pieza_actual->rotaciones[nr];
        int   h     = static_cast<int>(forma.size());
        int   w     = static_cast<int>(forma[0].size());

        for (int py = 0; py < h; ++py) {
            for (int px = 0; px < w; ++px) {
                if (forma[py][px] == 1) {
                    int wx = nx + px;
                    int wy = ny + py;
                    if (wx < 0 || wx >= ANCHO || wy >= ALTO) return false;
                    if (wy >= 0 && tablero[wy][wx] != VACIO) return false;
                }
            }
        }
        return true;
    }

    void fijarPieza() {
        if (!pieza_actual) return;
        vector<vector<int> >& forma = pieza_actual->rotaciones[pieza_actual->rotacion_actual];
        int   h     = static_cast<int>(forma.size());
        int   w     = static_cast<int>(forma[0].size());

        for (int py = 0; py < h; ++py) {
            for (int px = 0; px < w; ++px) {
                if (forma[py][px] == 1) {
                    int wx = pieza_actual->x + px;
                    int wy = pieza_actual->y + py;
                    if (wy >= 0 && wx >= 0 && wx < ANCHO && wy < ALTO) {
                        tablero[wy][wx]        = pieza_actual->tipo;
                        colores_tablero[wy][wx] = pieza_actual->color;
                    }
                }
            }
        }

        verificarLineasCompletas();
        generarNuevaPieza();
        generarSiguientePieza();
    }

    void verificarLineasCompletas() {
        vector<int> lineas;
        for (int y = 0; y < ALTO; ++y) {
            bool completa = true;
            for (int x = 0; x < ANCHO; ++x) {
                if (tablero[y][x] == VACIO) {
                    completa = false;
                    break;
                }
            }
            if (completa) {
                lineas.push_back(y);
            }
        }

        if (!lineas.empty()) {
            eliminarLineas(lineas);
            calcularPuntos(static_cast<int>(lineas.size()));
            lineas_completadas += static_cast<int>(lineas.size());

            int nivel_base = (config.nivel_inicial > 0) ? config.nivel_inicial : 1;
            int nuevo_nivel = (lineas_completadas / config.lineas_para_nivel) + nivel_base;
            if (nuevo_nivel > nivel) {
                nivel = nuevo_nivel;
                // decrease fall interval according to aceleracion_por_nivel
                int decrement = (nivel - config.nivel_inicial) * config.aceleracion_por_nivel;
                int nueva_vel = config.velocidad_inicial - decrement;
                if (nueva_vel < 50) nueva_vel = 50; // cap minimum interval
                velocidad_caida = nueva_vel;
            }
        }
    }

    void eliminarLineas(const vector<int>& lineas) {
        for (int i = static_cast<int>(lineas.size()) - 1; i >= 0; --i) {
            int linea = lineas[i];
            for (int y = linea; y > 0; --y) {
                for (int x = 0; x < ANCHO; ++x) {
                    tablero[y][x]        = tablero[y - 1][x];
                    colores_tablero[y][x] = colores_tablero[y - 1][x];
                }
            }
            for (int x = 0; x < ANCHO; ++x) {
                tablero[0][x]        = VACIO;
                colores_tablero[0][x] = GRIS;
            }
        }
    }

    void calcularPuntos(int n) {
        // Asegurar que el nivel mínimo sea 1 para el cálculo de puntos
        int nivel_para_puntos = (nivel > 0) ? nivel : 1;
        switch (n) {
            case 1: 
                puntos += config.puntos_linea_simple * nivel_para_puntos;
                contador_linea_simple++;
                break;
            case 2: 
                puntos += config.puntos_linea_doble * nivel_para_puntos;
                contador_linea_doble++;
                break;
            case 3: 
                puntos += config.puntos_linea_triple * nivel_para_puntos;
                contador_linea_triple++;
                break;
            case 4: 
                puntos += config.puntos_linea_tetris * nivel_para_puntos; // TETRIS
                contador_linea_tetris++;
                break;
        }
    }

    void procesarEntrada() {
        while (_kbhit()) {
            char tecla = static_cast<char>(tolower(_getch()));
            procesarTecla(tecla);
        }
    }

    void procesarTecla(char t) {
        if (game_over) {
            if (t == 'r') reiniciarJuego();
            return;
        }
        switch (t) {
            case 'a': {
                DWORD ahora = GetTickCount();
                if (ahora - last_horiz_move >= 120) {
                    if (esMovimientoValido(pieza_actual->x - 1, pieza_actual->y, pieza_actual->rotacion_actual)) pieza_actual->x--;
                    last_horiz_move = ahora;
                }
                break;
            }
            case 'd': {
                DWORD ahora = GetTickCount();
                if (ahora - last_horiz_move >= 120) {
                    if (esMovimientoValido(pieza_actual->x + 1, pieza_actual->y, pieza_actual->rotacion_actual)) pieza_actual->x++;
                    last_horiz_move = ahora;
                }
                break;
            }
            case 's': {
                // soft drop: descend one cell per key event (rate-limited)
                DWORD ahora = GetTickCount();
                if (ahora - last_soft_drop >= 80) {
                    if (esMovimientoValido(pieza_actual->x, pieza_actual->y + 1, pieza_actual->rotacion_actual)) {
                        pieza_actual->y++;
                    } else {
                        fijarPieza();
                    }
                    last_soft_drop = ahora;
                }
                break;
            }
            case 'w':
            case ' ': {
                static DWORD last_rotate = 0;
                DWORD ahoraR = GetTickCount();
                if (ahoraR - last_rotate >= 200) {
                    int nr = (pieza_actual->rotacion_actual + 1) % static_cast<int>(pieza_actual->rotaciones.size());
                    if (esMovimientoValido(pieza_actual->x, pieza_actual->y, nr)) {
                        pieza_actual->rotacion_actual = nr;
                    }
                    last_rotate = ahoraR;
                }
                break;
            }
            case 'p':
                pausado = !pausado;
                break;
            case 27: // ESC
            case 'q':
                juego_activo = false;
                break;
        }
    }

    void reiniciarJuego() {
        for (int y = 0; y < ALTO; ++y) {
            for (int x = 0; x < ANCHO; ++x) {
                tablero[y][x]        = VACIO;
                colores_tablero[y][x] = GRIS;
            }
        }
        puntos            = 0;
        nivel             = (config.nivel_inicial > 0) ? config.nivel_inicial : 1;
        lineas_completadas = 0;
        contador_linea_simple = 0;
        contador_linea_doble = 0;
        contador_linea_triple = 0;
        contador_linea_tetris = 0;
        velocidad_caida   = config.velocidad_inicial;
        game_over         = false;
        juego_activo      = true;
        pausado           = false;

        delete pieza_actual;
        delete siguiente_pieza;
        pieza_actual   = NULL;
        siguiente_pieza = NULL;

        generarNuevaPieza();
        generarSiguientePieza();

        DWORD ahora = GetTickCount();
        ultimo_movimiento = ahora;
        ultima_caida      = ahora;
    }

    void actualizarFisica() {
        if (pausado || game_over || !pieza_actual) return;
        DWORD ahora = GetTickCount();
        DWORD dt    = ahora - ultima_caida;
        if (dt >= (DWORD)velocidad_caida) {
            if (esMovimientoValido(pieza_actual->x, pieza_actual->y + 1, pieza_actual->rotacion_actual)) {
                pieza_actual->y++;
            } else {
                fijarPieza();
            }
            ultima_caida = ahora;
        }
    }

    void renderizar() {
        stringstream buf;
        buf << "\033[2J\033[H";
        buf << "\033[37;1m=== Tetris ===           Controles:\033[0m\n";
        buf << "\033[33mPuntos: " << puntos << "                      A/D - Mover\033[0m\n";
        buf << "\033[33mNivel: "  << nivel  << "                        S - Caida rapida\033[0m\n";
        buf << "\033[33mLineas: " << lineas_completadas << "                     W/SPACE - Rotar\033[0m\n";
        buf << "\033[90m                                  P - Pausa, ESC - Salir\033[0m\n\n";

        if (game_over) {
            buf << "\033[91m    !GAME OVER!\033[0m\n";
            buf << "\033[91m    Puntuacion Final: " << puntos << "\033[0m\n";
            buf << "\033[91m    Presiona R para reiniciar\033[0m\n\n";
        }
        if (pausado) {
            buf << "\033[95;1m    *** PAUSADO ***\033[0m\n";
            buf << "\033[95m    Presiona P para continuar\033[0m\n\n";
        }

        // Marco superior
        buf << "\033[90m         =========================\033[0m\n";

        for (int y = 0; y < ALTO; ++y) {
            buf << "\033[90m         |\033[0m";
            for (int x = 0; x < ANCHO; ++x) {
                bool   pieza   = false;
                char   simbolo = ' ';
                string cansi   = "\033[37m";

                // Pieza actual
                if (pieza_actual && !pausado && !game_over) {
                    vector<vector<int> >& f = pieza_actual->rotaciones[pieza_actual->rotacion_actual];
                    int py = y - pieza_actual->y;
                    int px = x - pieza_actual->x;
                    if (py >= 0 && py < static_cast<int>(f.size()) &&
                        px >= 0 && px < static_cast<int>(f[0].size()) &&
                        f[py][px] == 1) {
                        simbolo = (char)219;
                        cansi   = obtenerColorAnsi(static_cast<int>(pieza_actual->color));
                        pieza   = true;
                    }
                }

                // Fija/Tablero
                if (!pieza) {
                    if (tablero[y][x] != VACIO) {
                        simbolo = (char)219;
                        cansi   = obtenerColorAnsi(static_cast<int>(colores_tablero[y][x]));
                    } else {
                        simbolo = '.';
                        cansi   = "\033[90m";
                    }
                }

                buf << cansi << simbolo << simbolo << "\033[0m";
            }

            buf << "\033[90m|\033[0m";

            // Vista previa de la siguiente pieza
            if (y == 2) {
                buf << "\033[97m  Siguiente:\033[0m";
            } else if (y >= 4 && y <= 7 && siguiente_pieza) {
                buf << "  ";
                vector<vector<int> >& f = siguiente_pieza->rotaciones[0];
                int   py = y - 4;
                if (py < static_cast<int>(f.size())) {
                    string cansi = obtenerColorAnsi(static_cast<int>(siguiente_pieza->color));
                    for (int px = 0; px < min(4, static_cast<int>(f[0].size())); ++px) {
                        if (px < static_cast<int>(f[0].size()) && f[py][px] == 1) {
                            buf << cansi << (char)219 << (char)219 << "\033[0m";
                        } else {
                            buf << "  ";
                        }
                    }
                }
            }
            buf << "\n";
        }

        // Marco inferior
        buf << "\033[90m         =========================\033[0m\n";

        // FPS (medido en ms con GetTickCount)
        static DWORD ultimo_fps = GetTickCount();
        static int  fps        = 0;
        fps++;
        DWORD ahora_fps = GetTickCount();
        DWORD ms    = ahora_fps - ultimo_fps;
        if (ms >= 1000) {
            buf << "\033[92mFPS: " << fps << " | Cargado desde AST\033[0m\n";
            fps = 0;
            ultimo_fps = ahora_fps;
        }

        cout << buf.str();
        cout.flush();
    }

    void ejecutar() {
        const int TARGET_FPS = 60;
        const int FRAME_MS   = 1000 / TARGET_FPS;
        DWORD proximo = GetTickCount() + FRAME_MS;
        while (juego_activo) {
            DWORD inicio = GetTickCount();
            procesarEntrada();
            if (!pausado && !game_over) {
                actualizarFisica();
            }
            if (inicio >= proximo) {
                renderizar();
                proximo += FRAME_MS;
            }
            DWORD now = GetTickCount();
            if (proximo > now) {
                Sleep(proximo - now);
            }
        }
    }
};

// ================================================================
// Snake
// ================================================================
struct Posicion {
    int x;
    int y;
    Posicion(int x_ = 0, int y_ = 0) : x(x_), y(y_) {}
    bool operator==(const Posicion& o) const { return x == o.x && y == o.y; }
};

class SnakeEngine {
private:
    // Configuración y estado
    SnakeAST           config;
    ColorConsola       colorConsola;
    vector<Posicion>   cuerpo_snake;
    Posicion           direccion_actual;
    Posicion           fruta_posicion;
    string             fruta_tipo_actual;  // Tipo de fruta actual (manzana, cereza, etc.)
    bool               juego_activo;
    bool               pausado;
    bool               game_over;
    int                puntos;
    int                nivel;
    // Contadores de frutas comidas
    int                contador_manzana;
    int                contador_cereza;
    int                contador_banana;
    int                contador_uva;
    int                contador_naranja;
    int                total_frutas_comidas;  // Total de todas las frutas comidas
    // se usará rand() en lugar de mt19937 para compatibilidad C++03
    int                ancho_tablero;
    int                alto_tablero;
    int                velocidad_ms;
    string             nombre_juego;
public:
    SnakeEngine()
        : juego_activo(true),
         pausado(false),
         game_over(false),
         puntos(0),
         nivel(1),
         contador_manzana(0),
         contador_cereza(0),
         contador_banana(0),
         contador_uva(0),
         contador_naranja(0),
         total_frutas_comidas(0) {
        // semilla para rand()
        srand((unsigned)time(NULL));
        cargarConfiguracionAST();
        inicializarJuego();
    }
private:
    void cargarConfiguracionAST() {
        if (!config.cargarDesdeAST("build/arbol.ast")) {
            nombre_juego  = "Snake Clasico";
            ancho_tablero = 25;
            alto_tablero  = 20;
            velocidad_ms  = 150;
            return;
        }
        nombre_juego  = config.strings.count("nombre_juego")  ? config.strings["nombre_juego"]  : "Snake Clásico";
        ancho_tablero = config.integers.count("ancho_tablero") ? config.integers["ancho_tablero"] : 25;
        alto_tablero  = config.integers.count("alto_tablero")  ? config.integers["alto_tablero"]  : 20;
        velocidad_ms  = config.integers.count("velocidad_inicial") ? config.integers["velocidad_inicial"] : 150;
    }

    void inicializarJuego() {
        cuerpo_snake.clear();
        int cx = ancho_tablero / 2;
        int cy = alto_tablero / 2;
        int longitud = config.integers.count("longitud_inicial") ? config.integers["longitud_inicial"] : 3;
        for (int i = 0; i < longitud; ++i) {
            cuerpo_snake.push_back(Posicion(cx - i, cy));
        }
        direccion_actual = Posicion(1, 0);
        fruta_tipo_actual = "manzana";  // Inicializar tipo de fruta
        generarNuevaFruta();
        puntos    = 0;
        game_over = false;
        // Resetear contadores de frutas
        contador_manzana = 0;
        contador_cereza = 0;
        contador_banana = 0;
        contador_uva = 0;
        contador_naranja = 0;
        total_frutas_comidas = 0;
    }

    void generarNuevaFruta() {
        int minx = 1;
        int maxx = ancho_tablero - 2;
        int miny = 1;
        int maxy = alto_tablero - 2;
        do {
            int rx = (rand() % (maxx - minx + 1)) + minx;
            int ry = (rand() % (maxy - miny + 1)) + miny;
            fruta_posicion = Posicion(rx, ry);
        } while (esPosicionOcupadaPorSnake(fruta_posicion));
        
        // Seleccionar tipo de fruta aleatorio
        vector<string> frutas_disponibles;
        if (config.arrays.count("frutas_disponibles")) {
            frutas_disponibles = config.arrays["frutas_disponibles"];
        } else {
            // Frutas por defecto
            frutas_disponibles.push_back("manzana");
            frutas_disponibles.push_back("cereza");
            frutas_disponibles.push_back("banana");
        }
        
        if (!frutas_disponibles.empty()) {
            fruta_tipo_actual = frutas_disponibles[rand() % frutas_disponibles.size()];
        } else {
            fruta_tipo_actual = "manzana";  // Por defecto
        }
    }

    bool esPosicionOcupadaPorSnake(const Posicion& p) {
        for (size_t i = 0; i < cuerpo_snake.size(); ++i) {
            if (cuerpo_snake[i] == p) return true;
        }
        return false;
    }

    bool hayColisionConCuerpo(const Posicion& p) {
        // Verificar que haya al menos 2 segmentos (cabeza + cuerpo)
        if (cuerpo_snake.size() < 2) return false;
        
        // Verificar colisión con el cuerpo (excluyendo la cabeza en índice 0)
        // IMPORTANTE: Excluimos la cola (último segmento) porque se eliminará al moverse
        // Si la nueva cabeza está en la posición de la cola, NO es una colisión
        // Solo verificamos colisión con segmentos que permanecerán después del movimiento
        size_t ultimo_indice = cuerpo_snake.size() - 1;  // Índice de la cola
        for (size_t i = 1; i < ultimo_indice; ++i) {
            if (cuerpo_snake[i] == p) return true;
        }
        return false;
    }

    bool estaFueraDelTablero(const Posicion& p) {
        // Verificar límites del tablero
        // La serpiente puede moverse en el área interior: x de 1 a ancho-2, y de 1 a alto-2
        // Los bordes (x=0, x=ancho-1, y=0, y=alto-1) son paredes
        return p.x < 1 || p.x >= ancho_tablero - 1 || p.y < 1 || p.y >= alto_tablero - 1;
    }

    void procesarEntrada() {
        if (_kbhit()) {
            char t = _getch();
            if (game_over) {
                if (t == 'r' || t == 'R') {
                    inicializarJuego();
                } else if (t == 27) { // ESC
                    juego_activo = false;
                }
                return;
            }
            switch (t) {
                case 'w': case 'W': if (direccion_actual.y == 0) direccion_actual = Posicion(0, -1); break;
                case 's': case 'S': if (direccion_actual.y == 0) direccion_actual = Posicion(0,  1); break;
                case 'a': case 'A': if (direccion_actual.x == 0) direccion_actual = Posicion(-1, 0); break;
                case 'd': case 'D': if (direccion_actual.x == 0) direccion_actual = Posicion( 1, 0); break;
                case 'p': case 'P': pausado = !pausado; break;
                case 27:             juego_activo = false;   break; // ESC
            }
        }
    }

    void actualizarFisica() {
        if (pausado || game_over) return;
        
        // Verificar que la serpiente tenga al menos un segmento
        if (cuerpo_snake.empty()) {
            game_over = true;
            return;
        }
        
        // Verificar que la dirección sea válida (no sea (0,0))
        if (direccion_actual.x == 0 && direccion_actual.y == 0) {
            return; // No moverse si no hay dirección
        }
        
        // Calcular nueva posición de la cabeza
        Posicion nueva(cuerpo_snake[0].x + direccion_actual.x,
                       cuerpo_snake[0].y + direccion_actual.y);

        // Verificar configuración de colisiones
        bool fin_borde = config.booleans.count("terminar_al_chocar_borde")
                        ? config.booleans["terminar_al_chocar_borde"]
                        : true;
        bool fin_cuerpo = config.booleans.count("terminar_al_chocar_cuerpo")
                        ? config.booleans["terminar_al_chocar_cuerpo"]
                        : true;

        // Verificar colisión con bordes (mejorado)
        if (fin_borde) {
            // Verificar límites exactos del tablero
            if (nueva.x < 1 || nueva.x >= ancho_tablero - 1 || 
                nueva.y < 1 || nueva.y >= alto_tablero - 1) {
                game_over = true;
                return;
            }
        } else {
            // Si no termina al chocar borde, hacer wrap-around (teleportar al otro lado)
            if (nueva.x < 1) nueva.x = ancho_tablero - 2;
            else if (nueva.x >= ancho_tablero - 1) nueva.x = 1;
            if (nueva.y < 1) nueva.y = alto_tablero - 2;
            else if (nueva.y >= alto_tablero - 1) nueva.y = 1;
        }
        
        // Verificar colisión con el cuerpo (mejorado)
        // IMPORTANTE: Solo verificamos colisión con segmentos que permanecerán después del movimiento
        // La cola (último segmento) se eliminará, así que no cuenta como colisión
        if (fin_cuerpo) {
            // Si hay menos de 2 segmentos, no puede haber colisión con el cuerpo
            if (cuerpo_snake.size() >= 2) {
                // Verificar colisión con todos los segmentos excepto la cabeza (índice 0) y la cola (último índice)
                size_t ultimo_indice = cuerpo_snake.size() - 1;
                for (size_t i = 1; i < ultimo_indice; ++i) {
                    if (cuerpo_snake[i] == nueva) {
                        game_over = true;
                        return;
                    }
                }
            }
        }

        cuerpo_snake.insert(cuerpo_snake.begin(), nueva);
        if (nueva == fruta_posicion) {
            // Calcular puntos según el tipo de fruta
            int pf = config.integers.count("puntos_por_fruta")
                   ? config.integers["puntos_por_fruta"]
                   : 10;
            
            // Buscar puntos específicos del tipo de fruta en el AST
            // Formato esperado: "puntos_manzana", "puntos_cereza", etc.
            string clave_puntos = "puntos_" + fruta_tipo_actual;
            if (config.integers.count(clave_puntos)) {
                pf = config.integers[clave_puntos];
            } else {
                // Intentar obtener desde objetos anidados (si el parser lo soporta)
                // Por ahora usamos valores por defecto según el tipo
                if (fruta_tipo_actual == "manzana") pf = 10;
                else if (fruta_tipo_actual == "cereza") pf = 20;
                else if (fruta_tipo_actual == "banana") pf = 15;
                else if (fruta_tipo_actual == "uva") pf = 25;
                else if (fruta_tipo_actual == "naranja") pf = 30;
            }
            
            puntos += pf;
            // Incrementar contador según el tipo de fruta
            if (fruta_tipo_actual == "manzana") contador_manzana++;
            else if (fruta_tipo_actual == "cereza") contador_cereza++;
            else if (fruta_tipo_actual == "banana") contador_banana++;
            else if (fruta_tipo_actual == "uva") contador_uva++;
            else if (fruta_tipo_actual == "naranja") contador_naranja++;
            total_frutas_comidas++;  // Incrementar total
            
            // Aplicar efectos de la fruta
            // 1. Efecto de crecimiento (puede ser negativo para acortar)
            int crecimiento = 1;  // Por defecto crece 1
            string clave_crecimiento = "crecimiento_" + fruta_tipo_actual;
            if (config.integers.count(clave_crecimiento)) {
                crecimiento = config.integers[clave_crecimiento];
            } else {
                // Valores por defecto (actualizados según configuración)
                if (fruta_tipo_actual == "manzana") crecimiento = -1;  // Disminuye 1
                else if (fruta_tipo_actual == "cereza") crecimiento = 0;  // Sin crecimiento
                else if (fruta_tipo_actual == "banana") crecimiento = 0;  // Sin crecimiento
                else if (fruta_tipo_actual == "uva") crecimiento = 2;  // Aumenta 2
                else if (fruta_tipo_actual == "naranja") crecimiento = 1;  // Normal (crece 1)
            }
            
            // Aplicar crecimiento (si es negativo, acortar la serpiente)
            if (crecimiento > 0) {
                // Crecer: agregar segmentos al final
                for (int i = 0; i < crecimiento; ++i) {
                    if (!cuerpo_snake.empty()) {
                        Posicion ultimo = cuerpo_snake.back();
                        cuerpo_snake.push_back(ultimo);
                    }
                }
            } else if (crecimiento < 0) {
                // Acortar: eliminar segmentos del final (mínimo 1 segmento)
                int acortar = -crecimiento;
                for (int i = 0; i < acortar && cuerpo_snake.size() > 1; ++i) {
                    cuerpo_snake.pop_back();
                }
            }
            
            // 2. Efecto de velocidad (positivo = más lento, negativo = más rápido)
            int cambio_velocidad = 0;  // Por defecto sin cambio
            string clave_velocidad = "velocidad_" + fruta_tipo_actual;
            if (config.integers.count(clave_velocidad)) {
                cambio_velocidad = config.integers[clave_velocidad];
            } else {
                // Valores por defecto (actualizados según configuración)
                if (fruta_tipo_actual == "manzana") cambio_velocidad = 0;  // Sin cambio
                else if (fruta_tipo_actual == "cereza") cambio_velocidad = 20;  // Más lento
                else if (fruta_tipo_actual == "banana") cambio_velocidad = -15;  // Más rápido
                else if (fruta_tipo_actual == "uva") cambio_velocidad = 0;  // Sin cambio
                else if (fruta_tipo_actual == "naranja") cambio_velocidad = 0;  // Normal (sin cambio)
            }
            
            // Aplicar cambio de velocidad
            velocidad_ms += cambio_velocidad;
            // Límites de velocidad (más bajo = más rápido, más alto = más lento)
            int velocidad_min = config.integers.count("velocidad_minima") 
                              ? config.integers["velocidad_minima"] 
                              : 50;
            int velocidad_max = config.integers.count("velocidad_maxima") 
                              ? config.integers["velocidad_maxima"] 
                              : 500;
            if (velocidad_ms < velocidad_min) velocidad_ms = velocidad_min;
            if (velocidad_ms > velocidad_max) velocidad_ms = velocidad_max;
            
            generarNuevaFruta();
        } else {
            cuerpo_snake.pop_back();
        }
    }

    void renderizar() {
        stringstream buf;
        buf << "\033[2J\033[H";

        string titulo = config.strings.count("nombre_juego")
                       ? config.strings["nombre_juego"]
                       : string("Snake Clasico");

        buf << "\033[97;1m=== " << titulo << " ===\033[0m\n";
        buf << "\033[93mPuntos: " << puntos
            << " | Nivel: " << nivel
            << " | Longitud: " << cuerpo_snake.size()
            << " | Frutas: " << total_frutas_comidas
            << "\033[0m\n";
        buf << "\033[90mControles: WASD - Mover, P - Pausa, ESC - Salir\033[0m\n\n";

        if (game_over) {
            string msg = config.strings.count("mensaje_game_over")
                       ? config.strings["mensaje_game_over"]
                       : string("GAME OVER - Puntuación: {puntos}");
            size_t p = msg.find("{puntos}");
            if (p != string::npos) {
                std::ostringstream oss; oss << puntos;
                msg.replace(p, 8, oss.str());
            }
            buf << "\033[91;1m" << msg << "\033[0m\n";
            buf << "\033[91mPresiona R para reiniciar o ESC para salir\033[0m\n\n";
        }
        if (pausado) {
            string msg = config.strings.count("mensaje_pausa")
                       ? config.strings["mensaje_pausa"]
                       : string("PAUSA - Presiona P para continuar");
            buf << "\033[95;1m" << msg << "\033[0m\n\n";
        }

        // Marco superior
        buf << "\033[90m         ";
        for (int i = 0; i < ancho_tablero; ++i) {
            buf << ((i == 0 || i == ancho_tablero - 1) ? "+" : "=");
        }
        buf << "\033[0m\n";

        // Tablero
        for (int y = 0; y < alto_tablero; ++y) {
            buf << "\033[90m         |\033[0m";
            for (int x = 0; x < ancho_tablero; ++x) {
                char   ch  = ' ';
                string col = "\033[37m";
                if (Posicion(x, y) == fruta_posicion) {
                    // Obtener color según el tipo de fruta actual
                    string color_fruta = "rojo";  // Por defecto
                    
                    // Buscar color específico del tipo de fruta
                    string clave_color = "color_" + fruta_tipo_actual;
                    if (config.strings.count(clave_color)) {
                        color_fruta = config.strings[clave_color];
                    } else {
                        // Valores por defecto según tipo de fruta
                        if (fruta_tipo_actual == "manzana") color_fruta = "rojo";
                        else if (fruta_tipo_actual == "cereza") color_fruta = "rojo";
                        else if (fruta_tipo_actual == "banana") color_fruta = "amarillo";
                        else if (fruta_tipo_actual == "uva") color_fruta = "magenta";
                        else if (fruta_tipo_actual == "naranja") color_fruta = "naranja";
                        else {
                            // Fallback al color_fruta general
                            color_fruta = config.strings.count("color_fruta")
                                        ? config.strings["color_fruta"]
                                        : string("rojo");
                        }
                    }
                    
                    ch  = '@';
                    col = colorConsola.obtenerColorAnsi(color_fruta);
                } else {
                    bool es_cuerpo = false;
                    for (size_t i = 0; i < cuerpo_snake.size(); ++i) {
                        if (cuerpo_snake[i] == Posicion(x, y)) {
                            es_cuerpo = true;
                            vector<string> colores;
                            if (config.arrays.count("colores_snake")) {
                                colores = config.arrays["colores_snake"];
                            } else {
                                colores.push_back("verde_claro"); colores.push_back("verde_oscuro"); colores.push_back("verde_medio");
                            }
                            if (i == 0 && !colores.empty()) {
                                col = colorConsola.obtenerColorAnsi(colores[0]);
                                ch  = 'O';
                            } else {
                                if (colores.size() > 1) {
                                    col = colorConsola.obtenerColorAnsi(colores[1]);
                                }
                                ch = '#';
                            }
                            break;
                        }
                    }
                    if (!es_cuerpo) {
                        ch  = '.';
                        col = "\033[90m";
                    }
                }
                buf << col << ch << "\033[0m";
            }
            buf << "\033[90m|\033[0m\n";
        }

        // Marco inferior
        buf << "\033[90m         ";
        for (int i = 0; i < ancho_tablero; ++i) {
            buf << ((i == 0 || i == ancho_tablero - 1) ? "+" : "=");
        }
        buf << "\033[0m\n";

        buf << "\033[92mCargado desde AST: " << titulo << " | Configuracion completa desde Snake.brik\033[0m\n";

        cout << buf.str();
        cout.flush();
    }
public:
    void ejecutar() {
        DWORD proximo = GetTickCount() + velocidad_ms;
        while (juego_activo) {
            DWORD inicio = GetTickCount();
            procesarEntrada();
            if (inicio >= proximo) {
                actualizarFisica();
                proximo += velocidad_ms;
            }
            renderizar();
            Sleep(16); // Suaviza render (~60 FPS)
        }
    }
};

// ============================================================================
// FUNCIÓN: compilarJuegoSiPosible
// ============================================================================
// Intenta compilar el juego especificado antes de ejecutarlo.
// Esto asegura que el AST esté actualizado con la última configuración.
// @param juego Nombre del juego a compilar ("tetris" o "snake")
// ============================================================================
static void compilarJuegoSiPosible(const string& juego) {
    // Construir comando de compilación según el juego
    string comando;
    if (juego == "tetris") {
        comando = "bin\\compilador.exe config\\games\\Tetris.brik";
    } else if (juego == "snake") {
        comando = "bin\\compilador.exe config\\games\\Snake.brik";
    } else {
        return;  // Juego no reconocido
    }
    
    // Verificar que el compilador existe antes de ejecutarlo
    DWORD attrs = GetFileAttributesA("bin\\compilador.exe");
    if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        system(comando.c_str());
    }
}

// ============================================================================
// RENDERIZADOR GDI (WINDOWS) - Alternativa ligera compatible con Windows XP
// ============================================================================
// Motor de renderizado gráfico usando GDI (Graphics Device Interface) nativo
// de Windows. No requiere DLLs externas y es compatible con Windows XP.
//
// Características:
//   - Renderizado en ventana nativa de Windows
//   - Doble buffer para evitar parpadeo
//   - Manejo de entrada mediante GetAsyncKeyState
//   - Soporte completo para Tetris y Snake
//
// Compilar con: g++ -DUSE_GDI -o runtime runtime.cpp -lgdi32 -luser32
// ============================================================================
#ifdef USE_GDI

// Renderizador simple de ventana Win32 con doble buffer. Este es un prototipo
// minimalista diseñado para proporcionar una alternativa gráfica en Windows
// (incluyendo XP) usando GDI. Implementa renderizado básico y manejo de teclado
// para ambos juegos (Tetris y Snake). No es idéntico en características a
// renderizadores anteriores pero proporciona funcionalidad esencial.

// Variables globales para la ventana GDI
static HWND g_hWnd = NULL;      // Handle de la ventana principal
static bool g_running = true;    // Flag para controlar el bucle principal


LRESULT CALLBACK GDIWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            // La ventana se está destruyendo, terminar el bucle principal
            g_running = false;
            PostQuitMessage(0);
            return 0;
            
        case WM_CLOSE:
            // Cerrar la ventana
            DestroyWindow(hwnd);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/**
 * Crea una ventana GDI para renderizado gráfico.
 * @param title Título de la ventana
 * @param w Ancho de la ventana en píxeles
 * @param h Alto de la ventana en píxeles
 * @return true si la ventana se creó correctamente, false en caso contrario
 */
static bool createGDIWindow(const char* title, int w, int h) {
    WNDCLASSEXA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;  // Redibujar cuando cambia el tamaño
    wc.lpfnWndProc = (WNDPROC)GDIWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "LadrillosGDIClass";
    
    // Registrar la clase de ventana (puede fallar si ya está registrada)
    if (!RegisterClassExA(&wc)) {
        // La clase puede estar ya registrada, continuar de todas formas
    }
    
    // Crear la ventana
    g_hWnd = CreateWindowExA(0, wc.lpszClassName, title,
                             WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,  // Sin botón maximizar
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             w, h,
                             NULL, NULL, wc.hInstance, NULL);
    
    if (!g_hWnd) {
        return false;
    }
    
    // Mostrar y actualizar la ventana
    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);
    return true;
}

// ============================================================================
// FUNCIONES AUXILIARES PARA RENDERIZADO GDI
// ============================================================================

static void FillRectColor(HDC hdc, int x, int y, int w, int h, COLORREF col) {
    HBRUSH brush = CreateSolidBrush(col);
    RECT r = { x, y, x + w - 1, y + h - 1 };
    FillRect(hdc, &r, brush);
    DeleteObject(brush);
}

/**
 * Convierte valores RGB individuales a un COLORREF de Windows.
 * @param r Componente rojo (0-255)
 * @param g Componente verde (0-255)
 * @param b Componente azul (0-255)
 * @return COLORREF listo para usar en funciones GDI
 */
static COLORREF ColorRGB(int r, int g, int b) { 
    return RGB(r, g, b); 
}

// ============================================================================
// CLASE: TetrisEngineGDI
// ============================================================================
// Motor de Tetris con renderizado gráfico usando GDI (Graphics Device Interface).
// Esta implementación es compatible con Windows XP y no requiere DLLs externas.
// Utiliza ventanas nativas de Windows y funciones GDI para el renderizado.
// ============================================================================
class TetrisEngineGDI {
public:
    // Constantes del tablero de Tetris
    static const int ANCHO = 10;  // Ancho del tablero en celdas
    static const int ALTO  = 20;  // Alto del tablero en celdas
    
    // Configuración de renderizado
    int cell;      // Tamaño de cada celda en píxeles
    int offsetX;   // Desplazamiento horizontal del tablero en píxeles
    int offsetY;   // Desplazamiento vertical del tablero en píxeles

    // Estado del juego
    vector<vector<TipoPieza> > tablero;  // Matriz que representa el tablero
    PiezaTetris* pieza_actual;           // Pieza que está cayendo actualmente
    PiezaTetris* siguiente_pieza;        // Siguiente pieza que aparecerá
    ConfigTetris config;                  // Configuración cargada desde AST

    // Estadísticas y progreso
    int puntos;              // Puntuación actual del jugador
    int velocidad_caida;     // Velocidad de caída en milisegundos
    int nivel;               // Nivel actual del juego
    int lineas_completadas;  // Total de líneas completadas
    int contador_linea_simple;  // Contador de líneas simples completadas
    int contador_linea_doble;    // Contador de líneas dobles completadas
    int contador_linea_triple;   // Contador de líneas triples completadas
    int contador_linea_tetris;   // Contador de tetris (4 líneas) completados

    // Estados del juego
    bool juego_activo;  // Indica si el juego está en ejecución
    bool pausado;       // Indica si el juego está pausado
    bool game_over;     // Indica si el juego ha terminado

    // Temporizadores para control de movimiento
    DWORD ultima_caida;        // Último momento en que la pieza cayó automáticamente
    DWORD last_horiz_move;     // Último momento de movimiento horizontal
    DWORD last_soft_drop;     // Último momento de caída rápida (tecla S)
    DWORD last_rotate;         // Último momento de rotación
    
    // Estados anteriores de teclas para detección de flanco
    bool w_presionada_anterior;      // Estado anterior de la tecla W
    bool space_presionada_anterior;  // Estado anterior de la tecla Espacio

    /**
     * Constructor del motor de Tetris GDI.
     * @param forced_cell Tamaño forzado de celda en píxeles (0 = usar configuración AST)
     */
    TetrisEngineGDI(int forced_cell = 0) 
        : cell(24)
        , offsetX(20)
        , offsetY(20)
        , pieza_actual(NULL)
        , siguiente_pieza(NULL)
        , puntos(0)
        , velocidad_caida(800)
        , nivel(1)
        , lineas_completadas(0)
        , contador_linea_simple(0)
        , contador_linea_doble(0)
        , contador_linea_triple(0)
        , contador_linea_tetris(0)
        , juego_activo(true)
        , pausado(false)
        , game_over(false) {
        
        // Inicializar tablero vacío
        tablero.assign(ALTO, vector<TipoPieza>(ANCHO, VACIO));
        
        // Inicializar temporizadores
        DWORD ahora = GetTickCount();
        ultima_caida = ahora;
        last_horiz_move = ahora;
        last_soft_drop = ahora;
        last_rotate = ahora;
        
        // Inicializar estados de teclas
        w_presionada_anterior = false;
        space_presionada_anterior = false;
        
        // Inicializar generador de números aleatorios
        srand((unsigned)time(NULL));
        
        // Aplicar configuración desde AST
        if (forced_cell > 0) {
            cell = forced_cell;
        } else {
            cell = config.tamanio_celda;
        }
        velocidad_caida = config.velocidad_inicial;
        nivel = (config.nivel_inicial > 0) ? config.nivel_inicial : 1;
        
        // Generar primera pieza y siguiente pieza
        generarNuevaPieza();
        generarSiguientePieza();
    }

    /**
     * Destructor: libera la memoria de las piezas.
     */
    ~TetrisEngineGDI() { 
        delete pieza_actual;
        delete siguiente_pieza;
    }
    /**
     * Genera una nueva pieza que comenzará a caer.
     * Si hay una siguiente pieza preparada, la usa; si no, genera una aleatoria.
     */
    void generarNuevaPieza() {
        if (siguiente_pieza) {
            // Usar la siguiente pieza que ya estaba preparada
            delete pieza_actual;
            pieza_actual = siguiente_pieza;
            siguiente_pieza = NULL;
        } else {
            // Generar una nueva pieza aleatoria
            pieza_actual = new PiezaTetris(static_cast<TipoPieza>(rand() % 7), config);
        }
        
        // Posicionar la pieza en la parte superior central del tablero
        pieza_actual->x = ANCHO / 2 - 2;
        pieza_actual->y = 0;
        pieza_actual->rotacion_actual = 0;
        
        // Verificar si la pieza puede colocarse (si no, game over)
        if (!esMovimientoValido(pieza_actual->x, pieza_actual->y, pieza_actual->rotacion_actual)) {
            game_over = true;
            // NO cerrar el juego, permitir reiniciar
            // juego_activo se mantiene en true para que el bucle continúe
        }
    }

    /**
     * Genera la siguiente pieza que aparecerá después de la actual.
     * Esta pieza se prepara de antemano para una transición suave.
     */
    void generarSiguientePieza() {
        siguiente_pieza = new PiezaTetris(static_cast<TipoPieza>(rand() % 7), config);
    }
   
    bool esMovimientoValido(int nx, int ny, int nr) {
        if (!pieza_actual) {
            return false;
        }
        
        vector<vector<int> >& forma = pieza_actual->rotaciones[nr];
        int h = (int)forma.size();
        int w = (int)forma[0].size();

        // Verificar cada bloque de la pieza
        for (int py = 0; py < h; ++py) {
            for (int px = 0; px < w; ++px) {
                if (forma[py][px] == 1) {
                    int wx = nx + px;
                    int wy = ny + py;
                    
                    // Verificar límites del tablero
                    if (wx < 0 || wx >= ANCHO || wy >= ALTO) {
                        return false;
                    }
                    
                    // Verificar colisión con bloques ya colocados
                    if (wy >= 0 && tablero[wy][wx] != VACIO) {
                        return false;
                    }
                }
            }
        }
        
        return true;
    }
    /**
     * Fija la pieza actual en el tablero cuando no puede seguir cayendo.
     * Después de fijar, verifica líneas completas y genera nuevas piezas.
     */
    void fijarPieza() {
        if (!pieza_actual) {
            return;
        }
        
        // Obtener la forma actual de la pieza según su rotación
        vector<vector<int> >& forma = pieza_actual->rotaciones[pieza_actual->rotacion_actual];
        int h = (int)forma.size();
        int w = (int)forma[0].size();

        // Colocar cada bloque de la pieza en el tablero
        for (int py = 0; py < h; ++py) {
            for (int px = 0; px < w; ++px) {
                if (forma[py][px] == 1) {
                    int wx = pieza_actual->x + px;
                    int wy = pieza_actual->y + py;
                    
                    // Solo colocar si está dentro de los límites del tablero
                    if (wy >= 0 && wx >= 0 && wx < ANCHO && wy < ALTO) {
                        tablero[wy][wx] = pieza_actual->tipo;
                    }
                }
            }
        }
        
        // Verificar y eliminar líneas completas
        verificarLineasCompletas();
        
        // Generar nueva pieza y preparar la siguiente
        generarNuevaPieza();
        generarSiguientePieza();
    }

    /**
     * Elimina las líneas completas del tablero y hace caer las líneas superiores.
     * @param lineas Vector con los índices de las líneas a eliminar
     */
    void eliminarLineas(const vector<int>& lineas) {
        if (lineas.empty()) {
            return;
        }
        
        // Crear un vector de booleanos para marcar qué líneas eliminar
        vector<bool> marcar_eliminar(ALTO, false);
        for (size_t i = 0; i < lineas.size(); ++i) {
            if (lineas[i] >= 0 && lineas[i] < ALTO) {
                marcar_eliminar[lineas[i]] = true;
            }
        }
        
        // Crear nuevo tablero copiando solo las líneas que NO se eliminan
        vector<vector<TipoPieza> > nuevo_tablero(ALTO, vector<TipoPieza>(ANCHO, VACIO));
        int destino = ALTO - 1;
        
        // Copiar líneas de abajo hacia arriba, saltando las que se eliminan
        // Esto hace que las líneas superiores "caigan" automáticamente
        for (int origen = ALTO - 1; origen >= 0; --origen) {
            if (!marcar_eliminar[origen]) {
                // Esta línea se mantiene, copiarla a su nueva posición
                for (int x = 0; x < ANCHO; ++x) {
                    nuevo_tablero[destino][x] = tablero[origen][x];
                }
                destino--;
            }
        }
        
        // Las líneas superiores ya están con VACIO por defecto
        // Reemplazar el tablero antiguo con el nuevo
        tablero = nuevo_tablero;
    }

    /**
     * Verifica si hay líneas completas en el tablero y las elimina.
     * También actualiza la puntuación, nivel y velocidad según las líneas eliminadas.
     */
    void verificarLineasCompletas() {
        vector<int> lineas_completas;
        
        // Revisar cada línea del tablero
        for (int y = 0; y < ALTO; ++y) {
            bool completa = true;
            
            // Verificar si todos los espacios de la línea están ocupados
            for (int x = 0; x < ANCHO; ++x) {
                if (tablero[y][x] == VACIO) {
                    completa = false;
                    break;
                }
            }
            
            // Si la línea está completa, agregarla a la lista
            if (completa) {
                lineas_completas.push_back(y);
            }
        }
        
        // Si hay líneas completas, procesarlas
        if (!lineas_completas.empty()) {
            // Eliminar las líneas del tablero
            eliminarLineas(lineas_completas);
            
            // Calcular puntos según el número de líneas eliminadas
            calcularPuntos((int)lineas_completas.size());
            
            // Actualizar contador de líneas completadas
            lineas_completadas += (int)lineas_completas.size();
            
            // Calcular nuevo nivel basado en líneas completadas
            int nivel_base = (config.nivel_inicial > 0) ? config.nivel_inicial : 1;
            int nuevo_nivel = (lineas_completadas / config.lineas_para_nivel) + nivel_base;
            
            // Si subió de nivel, aumentar la velocidad
            if (nuevo_nivel > nivel) {
                nivel = nuevo_nivel;
                
                // Calcular nueva velocidad (más rápida = menor intervalo)
                int decrement = (nivel - config.nivel_inicial) * config.aceleracion_por_nivel;
                int nueva_vel = config.velocidad_inicial - decrement;
                
                // Velocidad mínima para evitar que sea imposible
                if (nueva_vel < 50) {
                    nueva_vel = 50;
                }
                
                velocidad_caida = nueva_vel;
            }
        }
    }

    /**
     * Calcula y suma los puntos obtenidos al eliminar líneas.
     * Los puntos se multiplican por el nivel actual para mayor dificultad = más puntos.
     * @param num_lineas Número de líneas eliminadas simultáneamente (1-4)
     */
    void calcularPuntos(int num_lineas) {
        // Asegurar que el nivel mínimo sea 1 para el cálculo de puntos
        int nivel_para_puntos = (nivel > 0) ? nivel : 1;
        switch (num_lineas) {
            case 1:
                puntos += config.puntos_linea_simple * nivel_para_puntos;
                contador_linea_simple++;
                break;
            case 2:
                puntos += config.puntos_linea_doble * nivel_para_puntos;
                contador_linea_doble++;
                break;
            case 3:
                puntos += config.puntos_linea_triple * nivel_para_puntos;
                contador_linea_triple++;
                break;
            case 4:
                puntos += config.puntos_linea_tetris * nivel_para_puntos;  // Tetris (4 líneas)
                contador_linea_tetris++;
                break;
            default:
                // Si hay más de 4 líneas (no debería pasar), usar el máximo
                puntos += config.puntos_linea_tetris * nivel_para_puntos;
                contador_linea_tetris++;
                break;
        }
    }

    /**
     * Reinicia el juego a su estado inicial.
     * Limpia el tablero, resetea estadísticas y genera nuevas piezas.
     */
    void reiniciarJuego() {
        // Limpiar el tablero
        for (int y = 0; y < ALTO; ++y) {
            for (int x = 0; x < ANCHO; ++x) {
                tablero[y][x] = VACIO;
            }
        }
        
        // Resetear estadísticas
        puntos = 0;
        nivel = (config.nivel_inicial > 0) ? config.nivel_inicial : 1;
        lineas_completadas = 0;
        contador_linea_simple = 0;
        contador_linea_doble = 0;
        contador_linea_triple = 0;
        contador_linea_tetris = 0;
        velocidad_caida = config.velocidad_inicial;
        
        // Resetear estados del juego
        game_over = false;
        juego_activo = true;
        pausado = false;

        // Liberar piezas actuales
        delete pieza_actual;
        delete siguiente_pieza;
        pieza_actual = NULL;
        siguiente_pieza = NULL;

        // Reiniciar temporizadores
        DWORD ahora = GetTickCount();
        ultima_caida = ahora;
        last_horiz_move = ahora;
        last_soft_drop = ahora;
        last_rotate = ahora;
        
        // Resetear estados de teclas
        w_presionada_anterior = false;
        space_presionada_anterior = false;
        
        // Generar nuevas piezas
        generarNuevaPieza();
        generarSiguientePieza();
    }
    /**
     * Actualiza la física del juego: hace caer la pieza automáticamente.
     * Se ejecuta en cada frame del juego.
     */
    void actualizarFisica() {
        // No actualizar si el juego está pausado o terminado
        if (pausado || game_over) {
            return;
        }

        DWORD ahora = GetTickCount();
        
        // Verificar si ha pasado el tiempo suficiente para que la pieza caiga
        if (ahora - ultima_caida >= (DWORD)velocidad_caida) {
            // Intentar mover la pieza hacia abajo
            if (esMovimientoValido(pieza_actual->x, pieza_actual->y + 1, pieza_actual->rotacion_actual)) {
                pieza_actual->y++;
            } else {
                // Si no puede bajar más, fijar la pieza en el tablero
                fijarPieza();
            }
            
            ultima_caida = ahora;
        }
    }
    /**
     * Procesa todas las entradas del teclado del jugador.
     * Maneja movimiento horizontal, rotación, caída rápida, pausa y reinicio.
     */
    void procesarTeclas() {
        // Reiniciar juego si está en game over
        if (game_over && (GetAsyncKeyState('R') & 0x8000)) {
            reiniciarJuego();
            Sleep(200);  // Pequeño delay para evitar múltiples reinicios
            return;
        }
        
        // No procesar movimiento si está pausado o en game over
        if (pausado || game_over) {
            if (GetAsyncKeyState('P') & 0x8000) { 
                if (!game_over) {
                    pausado = !pausado; 
                    Sleep(200); 
                }
            }
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) juego_activo = false;
            return;
        }
        
        DWORD ahora = GetTickCount();
        
        // ============================================================
        // MOVIMIENTO HORIZONTAL (A = izquierda, D = derecha)
        // ============================================================
        // Rate limiting: solo permite un movimiento cada 120ms
        if ((GetAsyncKeyState('A') & 0x8000) && (ahora - last_horiz_move >= 120)) {
            if (esMovimientoValido(pieza_actual->x - 1, pieza_actual->y, pieza_actual->rotacion_actual)) {
                pieza_actual->x--;
            }
            last_horiz_move = ahora;
        }
        
        if ((GetAsyncKeyState('D') & 0x8000) && (ahora - last_horiz_move >= 120)) {
            if (esMovimientoValido(pieza_actual->x + 1, pieza_actual->y, pieza_actual->rotacion_actual)) {
                pieza_actual->x++;
            }
            last_horiz_move = ahora;
        }
        
        // ============================================================
        // CAÍDA RÁPIDA (S = bajar manualmente)
        // ============================================================
        // Rate limiting: solo permite un movimiento cada 80ms
        if ((GetAsyncKeyState('S') & 0x8000) && (ahora - last_soft_drop >= 80)) {
            if (esMovimientoValido(pieza_actual->x, pieza_actual->y + 1, pieza_actual->rotacion_actual)) {
                pieza_actual->y++;
            } else {
                // Si no puede bajar más, fijar la pieza
                fijarPieza();
            }
            last_soft_drop = ahora;
        }
        
        // ============================================================
        // ROTACIÓN (W o Espacio = rotar pieza)
        // ============================================================
        // Usar detección de flanco para evitar rotaciones continuas
        bool w_actual = (GetAsyncKeyState('W') & 0x8000) != 0;
        bool space_actual = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
        
        // Rotar solo cuando la tecla cambia de no presionada a presionada (flanco de subida)
        if ((w_actual && !w_presionada_anterior) || (space_actual && !space_presionada_anterior)) {
            DWORD ahoraR = GetTickCount();
            
            // Delay mínimo de 150ms para evitar rotaciones demasiado rápidas
            if (ahoraR - last_rotate >= 150) {
                // Calcular siguiente rotación (cíclica)
                int siguiente_rotacion = (pieza_actual->rotacion_actual + 1) % (int)pieza_actual->rotaciones.size();
                
                // Solo rotar si el movimiento es válido
                if (esMovimientoValido(pieza_actual->x, pieza_actual->y, siguiente_rotacion)) {
                    pieza_actual->rotacion_actual = siguiente_rotacion;
                }
                
                last_rotate = ahoraR;
            }
        }
        
        // Actualizar estado anterior de las teclas para la próxima iteración
        w_presionada_anterior = w_actual;
        space_presionada_anterior = space_actual;
        
        // ============================================================
        // CONTROLES DEL JUEGO
        // ============================================================
        // Reiniciar juego (también funciona durante el juego)
        if (GetAsyncKeyState('R') & 0x8000) {
            reiniciarJuego();
            Sleep(200);  // Pequeño delay para evitar múltiples reinicios
        }
        
        // Pausar/despausar juego
        if (GetAsyncKeyState('P') & 0x8000) {
            pausado = !pausado;
            Sleep(200);  // Pequeño delay para evitar múltiples cambios
        }
        
        // Salir del juego
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            juego_activo = false;
        }
    }
    /**
     * Renderiza todo el juego en el contexto de dispositivo especificado.
     * Dibuja el tablero, la pieza actual, el HUD y la información de configuración.
     * @param hdc Contexto de dispositivo donde dibujar
     */
    void renderizar(HDC hdc) {
        // Limpiar el fondo con color oscuro
        FillRectColor(hdc, 0, 0, 800, 700, ColorRGB(20, 20, 30));
        
        // Calcular dimensiones del tablero
        int boardW = ANCHO * cell;
        int boardH = ALTO * cell;
        
        // Dibujar borde del tablero (gris oscuro)
        FillRectColor(hdc, offsetX - 2, offsetY - 2, boardW + 4, boardH + 4, ColorRGB(100, 100, 100));
        
        // Dibujar fondo del tablero (gris muy oscuro)
        FillRectColor(hdc, offsetX, offsetY, boardW, boardH, ColorRGB(40, 40, 50));
        
        // Dibujar celdas del tablero
        for (int y = 0; y < ALTO; ++y) {
            for (int x = 0; x < ANCHO; ++x) {
                COLORREF color_celda = ColorRGB(30, 30, 40);  // Color por defecto (vacío)
                
                // Si la celda está ocupada, usar el color de la pieza
                if (tablero[y][x] != VACIO) {
                    // Convertir TipoPieza a string para buscar el color
                    string tipo_str;
                    switch (tablero[y][x]) {
                        case I: tipo_str = "I"; break;
                        case J: tipo_str = "J"; break;
                        case L: tipo_str = "L"; break;
                        case O: tipo_str = "O"; break;
                        case S: tipo_str = "S"; break;
                        case Z: tipo_str = "Z"; break;
                        case T: tipo_str = "T"; break;
                        default: tipo_str = "I"; break;
                    }
                    vector<int> rgb = config.obtenerColorRGB(tipo_str);
                    if (rgb.size() >= 3) {
                        color_celda = ColorRGB(rgb[0], rgb[1], rgb[2]);
                    } else {
                        color_celda = ColorRGB(100, 200, 100); // Verde por defecto
                    }
                }
                
                // Dibujar celda con pequeño margen para efecto visual
                FillRectColor(hdc, 
                    offsetX + x * cell + 1, 
                    offsetY + y * cell + 1, 
                    cell - 2, 
                    cell - 2, 
                    color_celda);
            }
        }
        
        // Dibujar pieza actual que está cayendo
        if (pieza_actual) {
            vector<vector<int> >& forma = pieza_actual->rotaciones[pieza_actual->rotacion_actual];
            
            for (int py = 0; py < (int)forma.size(); ++py) {
                for (int px = 0; px < (int)forma[0].size(); ++px) {
                    if (forma[py][px] == 1) {
                        int bx = pieza_actual->x + px;
                        int by = pieza_actual->y + py;
                        
                        // Solo dibujar si la pieza está dentro del tablero visible
                        if (by >= 0) {
                            // Usar el color RGB de la pieza desde la configuración
                            vector<int> rgb = config.obtenerColorRGB(pieza_actual->tipo_str);
                            COLORREF color_pieza;
                            if (rgb.size() >= 3) {
                                color_pieza = ColorRGB(rgb[0], rgb[1], rgb[2]);
                            } else {
                                color_pieza = ColorRGB(200, 100, 50); // Naranja por defecto
                            }
                            FillRectColor(hdc, 
                                offsetX + bx * cell + 1, 
                                offsetY + by * cell + 1, 
                                cell - 2, 
                                cell - 2, 
                                color_pieza);
                        }
                    }
                }
            }
        }
        
        // Configurar texto
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        
        // Panel de información a la derecha
        int panelX = offsetX + boardW + 20;
        int panelY = offsetY;
        int lineHeight = 20;
        int currentY = panelY;
        
        // Título
        TextOutA(hdc, panelX, currentY, "TETRIS", 6);
        currentY += lineHeight + 10;
        
        // Puntos totales
        char buf[128];
        sprintf(buf, "Puntos: %d", puntos);
        TextOutA(hdc, panelX, currentY, buf, (int)strlen(buf));
        currentY += lineHeight;
        
        // Nivel
        sprintf(buf, "Nivel: %d", nivel);
        TextOutA(hdc, panelX, currentY, buf, (int)strlen(buf));
        currentY += lineHeight;
        
        // Líneas completadas
        sprintf(buf, "Lineas: %d", lineas_completadas);
        TextOutA(hdc, panelX, currentY, buf, (int)strlen(buf));
        currentY += lineHeight + 15;
        
        // Tabla de puntajes por tipo de línea
        TextOutA(hdc, panelX, currentY, "Puntos por linea:", 17);
        currentY += lineHeight;
        
        sprintf(buf, "1 linea: %d x%d", config.puntos_linea_simple, contador_linea_simple);
        TextOutA(hdc, panelX, currentY, buf, (int)strlen(buf));
        currentY += lineHeight;
        
        sprintf(buf, "2 lineas: %d x%d", config.puntos_linea_doble, contador_linea_doble);
        TextOutA(hdc, panelX, currentY, buf, (int)strlen(buf));
        currentY += lineHeight;
        
        sprintf(buf, "3 lineas: %d x%d", config.puntos_linea_triple, contador_linea_triple);
        TextOutA(hdc, panelX, currentY, buf, (int)strlen(buf));
        currentY += lineHeight;
        
        sprintf(buf, "4 lineas: %d x%d", config.puntos_linea_tetris, contador_linea_tetris);
        TextOutA(hdc, panelX, currentY, buf, (int)strlen(buf));
        currentY += lineHeight + 15;
        
        // Controles
        TextOutA(hdc, panelX, currentY, "Controles:", 10);
        currentY += lineHeight;
        TextOutA(hdc, panelX, currentY, "A/D - Mover", 11);
        currentY += lineHeight;
        TextOutA(hdc, panelX, currentY, "S - Bajar rapido", 16);
        currentY += lineHeight;
        TextOutA(hdc, panelX, currentY, "W/ESP - Rotar", 14);
        currentY += lineHeight;
        TextOutA(hdc, panelX, currentY, "P - Pausa", 9);
        currentY += lineHeight;
        TextOutA(hdc, panelX, currentY, "R - Reiniciar", 14);
        currentY += lineHeight;
        TextOutA(hdc, panelX, currentY, "ESC - Salir", 11);
        currentY += lineHeight;
        
        // Mensaje de game over
        if (game_over) {
            currentY += 10;
            SetTextColor(hdc, RGB(255, 100, 100));
            TextOutA(hdc, panelX, currentY, "GAME OVER!", 10);
            currentY += lineHeight;
            SetTextColor(hdc, RGB(255, 255, 255));
            TextOutA(hdc, panelX, currentY, "Presiona R para", 15);
            currentY += lineHeight;
            TextOutA(hdc, panelX, currentY, "reiniciar", 9);
        }
        
        // Mensaje de pausa
        if (pausado && !game_over) {
            currentY += 10;
            SetTextColor(hdc, RGB(255, 255, 100));
            TextOutA(hdc, panelX, currentY, "PAUSA", 5);
            currentY += lineHeight;
            SetTextColor(hdc, RGB(255, 255, 255));
            TextOutA(hdc, panelX, currentY, "Presiona P para", 15);
            currentY += lineHeight;
            TextOutA(hdc, panelX, currentY, "continuar", 9);
        }
        
        // Restaurar color de texto
        SetTextColor(hdc, RGB(255, 255, 255));
    }

    /**
     * Ejecuta el bucle principal del juego.
     * Maneja mensajes de Windows, procesa entrada, actualiza física y renderiza.
     * Usa doble buffer para evitar parpadeo.
     */
    void run() {
        // Crear contexto de dispositivo para la ventana
        HDC hdcWindow = GetDC(g_hWnd);
        
        // Crear contexto de memoria para doble buffer
        HDC memDC = CreateCompatibleDC(hdcWindow);
        HBITMAP hbm = CreateCompatibleBitmap(hdcWindow, 800, 700);
        HBITMAP oldbm = (HBITMAP)SelectObject(memDC, hbm);
        
        // Bucle principal del juego
        while (juego_activo && g_running) {
            // Procesar mensajes de Windows
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    juego_activo = false;
                    g_running = false;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            
            // Procesar entrada (incluye reinicio si hay game over)
            procesarTeclas();
            
            // Actualizar física solo si no está pausado ni en game over
            if (!pausado && !game_over) {
                actualizarFisica();
            }
            
            // Renderizar siempre (muestra game over si es necesario)
            renderizar(memDC);
            
            // Copiar buffer de memoria a la ventana (blit)
            BitBlt(hdcWindow, 0, 0, 800, 700, memDC, 0, 0, SRCCOPY);
            
            // Pequeña pausa para no saturar la CPU (~60 FPS)
            Sleep(16);
        }
        
        // Limpiar recursos de GDI
        SelectObject(memDC, oldbm);
        DeleteObject(hbm);
        DeleteDC(memDC);
        ReleaseDC(g_hWnd, hdcWindow);
    }
};

// --- Minimal Snake GDI ---
class SnakeEngineGDI {
public:
    int cell; int offsetX, offsetY; int ancho_tablero, alto_tablero; int velocidad_ms;
    vector<Posicion> cuerpo; Posicion fruta; Posicion direccion;
    string fruta_tipo_actual;  // Tipo de fruta actual para GDI
    bool juego_activo; bool pausado; bool game_over; int puntos;
    // Contadores de frutas comidas
    int contador_manzana, contador_cereza, contador_banana, contador_uva, contador_naranja;
    int total_frutas_comidas;  // Total de todas las frutas comidas
    DWORD ultima;
    SnakeAST config;
    SnakeEngineGDI(): cell(18), offsetX(20), offsetY(60), ancho_tablero(25), alto_tablero(20), velocidad_ms(150) {
        srand((unsigned)time(NULL)); juego_activo=true; pausado=false; game_over=false; puntos=0; ultima=GetTickCount();
        contador_manzana=0; contador_cereza=0; contador_banana=0; contador_uva=0; contador_naranja=0; total_frutas_comidas=0;
        cargarConfiguracion(); inicializarJuego();
    }
    void cargarConfiguracion() { if (!config.cargarDesdeAST("build/arbol.ast")) { ancho_tablero=25; alto_tablero=20; velocidad_ms=150; } else { ancho_tablero = config.integers.count("ancho_tablero")?config.integers["ancho_tablero"]:25; alto_tablero = config.integers.count("alto_tablero")?config.integers["alto_tablero"]:20; velocidad_ms = config.integers.count("velocidad_inicial")?config.integers["velocidad_inicial"]:150; } }
    void inicializarJuego() { 
        cuerpo.clear(); 
        int cx=ancho_tablero/2, cy=alto_tablero/2; 
        int len = config.integers.count("longitud_inicial")?config.integers["longitud_inicial"]:3; 
        for (int i=0;i<len;++i) cuerpo.push_back(Posicion(cx-i,cy)); 
        direccion = Posicion(1,0); 
        fruta_tipo_actual = "manzana";  // Inicializar tipo de fruta
        generarFruta(); 
        puntos=0; 
        game_over=false;
        // Resetear contadores de frutas
        contador_manzana=0; contador_cereza=0; contador_banana=0; contador_uva=0; contador_naranja=0; total_frutas_comidas=0; 
    }
    void generarFruta() { 
        do { 
            int rx = (rand()%(ancho_tablero-2))+1; 
            int ry = (rand()%(alto_tablero-2))+1; 
            fruta = Posicion(rx,ry); 
        } while (ocupada(fruta));
        
        // Seleccionar tipo de fruta aleatorio
        vector<string> frutas_disponibles;
        if (config.arrays.count("frutas_disponibles")) {
            frutas_disponibles = config.arrays["frutas_disponibles"];
        } else {
            frutas_disponibles.push_back("manzana");
            frutas_disponibles.push_back("cereza");
            frutas_disponibles.push_back("banana");
        }
        
        if (!frutas_disponibles.empty()) {
            fruta_tipo_actual = frutas_disponibles[rand() % frutas_disponibles.size()];
        } else {
            fruta_tipo_actual = "manzana";
        }
    }
    bool ocupada(const Posicion& p) { for (size_t i=0;i<cuerpo.size();++i) if (cuerpo[i]==p) return true; return false; }
    void procesarTeclas() {
        // Reiniciar juego si está en game over
        if (game_over && (GetAsyncKeyState('R') & 0x8000)) {
            inicializarJuego();
            Sleep(200);  // Pequeño delay para evitar múltiples reinicios
            return;
        }
        
        // No procesar movimiento si está pausado o en game over
        if (pausado || game_over) {
            if (GetAsyncKeyState('P') & 0x8000) { 
                if (!game_over) {
                    pausado = !pausado; 
                    Sleep(200); 
                }
            }
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) juego_activo=false;
            return;
        }
        
        // Controles de movimiento
        if (GetAsyncKeyState('W') & 0x8000) if (direccion.y==0) direccion = Posicion(0,-1);
        if (GetAsyncKeyState('S') & 0x8000) if (directionSafe(0,1)) direccion = Posicion(0,1);
        if (GetAsyncKeyState('A') & 0x8000) if (direccion.x==0) direccion = Posicion(-1,0);
        if (GetAsyncKeyState('D') & 0x8000) if (direccion.x==0) direccion = Posicion(1,0);
        
        // Pausa y salir
        if (GetAsyncKeyState('P') & 0x8000) { pausado = !pausado; Sleep(200); }
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) juego_activo=false;
    }
    bool directionSafe(int dx,int dy) { if (dx == -direccion.x && dy == -direccion.y) return false; return true; }
    void actualizarFisica() { 
        if (pausado||game_over) return; 
        DWORD ahora=GetTickCount(); 
        if (ahora - ultima < (DWORD)velocidad_ms) return; 
        ultima = ahora; 
        
        // Verificar que la serpiente tenga al menos un segmento
        if (cuerpo.empty()) {
            game_over = true;
            return;
        }
        
        // Verificar que la dirección sea válida (no sea (0,0))
        if (direccion.x == 0 && direccion.y == 0) {
            return; // No moverse si no hay dirección
        }
        
        Posicion nueva(cuerpo[0].x+direccion.x, cuerpo[0].y+direccion.y);
        
        // Verificar configuración de colisiones
        bool fin_borde = config.booleans.count("terminar_al_chocar_borde")
                        ? config.booleans["terminar_al_chocar_borde"]
                        : true;
        bool fin_cuerpo = config.booleans.count("terminar_al_chocar_cuerpo")
                        ? config.booleans["terminar_al_chocar_cuerpo"]
                        : true;
        
        // Verificar colisión con bordes (mejorado)
        if (fin_borde) {
            // Verificar límites exactos del tablero
            if (nueva.x < 1 || nueva.x >= ancho_tablero - 1 || 
                nueva.y < 1 || nueva.y >= alto_tablero - 1) {
                game_over = true;
                return;
            }
        } else {
            // Si no termina al chocar borde, hacer wrap-around (teleportar al otro lado)
            if (nueva.x < 1) nueva.x = ancho_tablero - 2;
            else if (nueva.x >= ancho_tablero - 1) nueva.x = 1;
            if (nueva.y < 1) nueva.y = alto_tablero - 2;
            else if (nueva.y >= alto_tablero - 1) nueva.y = 1;
        }
        
        // Verificar colisión con el cuerpo (mejorado)
        // IMPORTANTE: Solo verificamos colisión con segmentos que permanecerán después del movimiento
        // La cola (último segmento) se eliminará, así que no cuenta como colisión
        if (fin_cuerpo) {
            // Si hay menos de 2 segmentos, no puede haber colisión con el cuerpo
            if (cuerpo.size() >= 2) {
                // Verificar colisión con todos los segmentos excepto la cabeza (índice 0) y la cola (último índice)
                size_t ultimo_indice = cuerpo.size() - 1;
                for (size_t i = 1; i < ultimo_indice; ++i) {
                    if (cuerpo[i] == nueva) {
                        game_over = true;
                        return;
                    }
                }
            }
        } 
        cuerpo.insert(cuerpo.begin(), nueva); 
        if (nueva==fruta) { 
            // Calcular puntos según tipo de fruta
            int pf = config.integers.count("puntos_por_fruta")?config.integers["puntos_por_fruta"]:10;
            string clave_puntos = "puntos_" + fruta_tipo_actual;
            if (config.integers.count(clave_puntos)) {
                pf = config.integers[clave_puntos];
            } else {
                // Valores por defecto
                if (fruta_tipo_actual == "manzana") pf = 10;
                else if (fruta_tipo_actual == "cereza") pf = 20;
                else if (fruta_tipo_actual == "banana") pf = 15;
                else if (fruta_tipo_actual == "uva") pf = 25;
                else if (fruta_tipo_actual == "naranja") pf = 30;
            }
            puntos += pf;
            // Incrementar contador según el tipo de fruta
            if (fruta_tipo_actual == "manzana") contador_manzana++;
            else if (fruta_tipo_actual == "cereza") contador_cereza++;
            else if (fruta_tipo_actual == "banana") contador_banana++;
            else if (fruta_tipo_actual == "uva") contador_uva++;
            else if (fruta_tipo_actual == "naranja") contador_naranja++;
            total_frutas_comidas++;  // Incrementar total
            
            // Aplicar efectos de la fruta
            // 1. Efecto de crecimiento (puede ser negativo para acortar)
            int crecimiento = 1;  // Por defecto crece 1
            string clave_crecimiento = "crecimiento_" + fruta_tipo_actual;
            if (config.integers.count(clave_crecimiento)) {
                crecimiento = config.integers[clave_crecimiento];
            } else {
                // Valores por defecto (actualizados según configuración)
                if (fruta_tipo_actual == "manzana") crecimiento = -1;  // Disminuye 1
                else if (fruta_tipo_actual == "cereza") crecimiento = 0;  // Sin crecimiento
                else if (fruta_tipo_actual == "banana") crecimiento = 0;  // Sin crecimiento
                else if (fruta_tipo_actual == "uva") crecimiento = 2;  // Aumenta 2
                else if (fruta_tipo_actual == "naranja") crecimiento = 1;  // Normal (crece 1)
            }
            
            // Aplicar crecimiento (si es negativo, acortar la serpiente)
            if (crecimiento > 0) {
                // Crecer: agregar segmentos al final
                for (int i = 0; i < crecimiento; ++i) {
                    if (!cuerpo.empty()) {
                        Posicion ultimo = cuerpo.back();
                        cuerpo.push_back(ultimo);
                    }
                }
            } else if (crecimiento < 0) {
                // Acortar: eliminar segmentos del final (mínimo 1 segmento)
                int acortar = -crecimiento;
                for (int i = 0; i < acortar && cuerpo.size() > 1; ++i) {
                    cuerpo.pop_back();
                }
            }
            
            // 2. Efecto de velocidad (positivo = más lento, negativo = más rápido)
            int cambio_velocidad = 0;  // Por defecto sin cambio
            string clave_velocidad = "velocidad_" + fruta_tipo_actual;
            if (config.integers.count(clave_velocidad)) {
                cambio_velocidad = config.integers[clave_velocidad];
            } else {
                // Valores por defecto (actualizados según configuración)
                if (fruta_tipo_actual == "manzana") cambio_velocidad = 0;  // Sin cambio
                else if (fruta_tipo_actual == "cereza") cambio_velocidad = 20;  // Más lento
                else if (fruta_tipo_actual == "banana") cambio_velocidad = -15;  // Más rápido
                else if (fruta_tipo_actual == "uva") cambio_velocidad = 0;  // Sin cambio
                else if (fruta_tipo_actual == "naranja") cambio_velocidad = 0;  // Normal (sin cambio)
            }
            
            // Aplicar cambio de velocidad
            velocidad_ms += cambio_velocidad;
            // Límites de velocidad (más bajo = más rápido, más alto = más lento)
            int velocidad_min = config.integers.count("velocidad_minima") 
                              ? config.integers["velocidad_minima"] 
                              : 50;
            int velocidad_max = config.integers.count("velocidad_maxima") 
                              ? config.integers["velocidad_maxima"] 
                              : 500;
            if (velocidad_ms < velocidad_min) velocidad_ms = velocidad_min;
            if (velocidad_ms > velocidad_max) velocidad_ms = velocidad_max;
            
            generarFruta(); 
        } else {
            cuerpo.pop_back();
        }
    }
    /**
     * Obtiene los puntos de un tipo de fruta desde la configuración.
     */
    int obtenerPuntosFruta(const string& tipo) {
        string clave = "puntos_" + tipo;
        if (config.integers.count(clave)) {
            return config.integers[clave];
        }
        // Valores por defecto
        if (tipo == "manzana") return 10;
        else if (tipo == "cereza") return 20;
        else if (tipo == "banana") return 15;
        else if (tipo == "uva") return 25;
        else if (tipo == "naranja") return 30;
        return 10;
    }
    
    void renderizar(HDC hdc) {
        RECT r; 
        GetClientRect(g_hWnd, &r); 
        FillRectColor(hdc, 0, 0, r.right - r.left, r.bottom - r.top, ColorRGB(20, 20, 30));
        
        // Calcular dimensiones del tablero
        int bw = ancho_tablero * cell;
        int bh = alto_tablero * cell;
        
        // Dibujar borde del tablero
        FillRectColor(hdc, offsetX - 2, offsetY - 2, bw + 4, bh + 4, ColorRGB(100, 100, 100));
        FillRectColor(hdc, offsetX, offsetY, bw, bh, ColorRGB(30, 30, 40));
        
        // Dibujar fruta con color según tipo
        COLORREF color_fruta_rgb = ColorRGB(255, 80, 80);  // Rojo por defecto
        if (fruta_tipo_actual == "manzana" || fruta_tipo_actual == "cereza") {
            color_fruta_rgb = ColorRGB(255, 80, 80);  // Rojo
        } else if (fruta_tipo_actual == "banana") {
            color_fruta_rgb = ColorRGB(255, 255, 0);  // Amarillo
        } else if (fruta_tipo_actual == "uva") {
            color_fruta_rgb = ColorRGB(255, 0, 255);  // Magenta
        } else if (fruta_tipo_actual == "naranja") {
            color_fruta_rgb = ColorRGB(255, 165, 0);  // Naranja
        }
        FillRectColor(hdc, offsetX + fruta.x * cell + 2, offsetY + fruta.y * cell + 2, cell - 4, cell - 4, color_fruta_rgb);
        
        // Dibujar snake
        for (size_t i = 0; i < cuerpo.size(); ++i) {
            COLORREF col = (i == 0) ? ColorRGB(200, 255, 200) : ColorRGB(0, 150, 0);
            FillRectColor(hdc, offsetX + cuerpo[i].x * cell + 1, offsetY + cuerpo[i].y * cell + 1, cell - 2, cell - 2, col);
        }
        
        // Configurar texto
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        
        // Panel de información a la derecha
        int panelX = offsetX + bw + 20;
        int panelY = offsetY;
        int lineHeight = 20;
        int currentY = panelY;
        
        // Título
        TextOutA(hdc, panelX, currentY, "SNAKE", 5);
        currentY += lineHeight + 10;
        
        // Puntos totales
        char buf[128];
        sprintf(buf, "Puntos: %d", puntos);
        TextOutA(hdc, panelX, currentY, buf, (int)strlen(buf));
        currentY += lineHeight + 10;
        
        // Longitud
        sprintf(buf, "Longitud: %d", (int)cuerpo.size());
        TextOutA(hdc, panelX, currentY, buf, (int)strlen(buf));
        currentY += lineHeight;
        
        // Total de frutas comidas
        sprintf(buf, "Frutas comidas: %d", total_frutas_comidas);
        TextOutA(hdc, panelX, currentY, buf, (int)strlen(buf));
        currentY += lineHeight + 15;
        
        // Tabla de puntajes de frutas
        TextOutA(hdc, panelX, currentY, "Puntos por fruta:", 17);
        currentY += lineHeight;
        
        // Dibujar cada tipo de fruta con su color, puntos y contador
        vector<string> frutas;
        frutas.push_back("manzana");
        frutas.push_back("cereza");
        frutas.push_back("banana");
        frutas.push_back("uva");
        frutas.push_back("naranja");
        for (size_t i = 0; i < frutas.size(); ++i) {
            string tipo = frutas[i];
            int pts = obtenerPuntosFruta(tipo);
            
            // Obtener contador según el tipo de fruta
            int contador = 0;
            if (tipo == "manzana") contador = contador_manzana;
            else if (tipo == "cereza") contador = contador_cereza;
            else if (tipo == "banana") contador = contador_banana;
            else if (tipo == "uva") contador = contador_uva;
            else if (tipo == "naranja") contador = contador_naranja;
            
            // Dibujar cuadro de color de la fruta
            COLORREF color_fr = ColorRGB(255, 80, 80);
            if (tipo == "banana") color_fr = ColorRGB(255, 255, 0);
            else if (tipo == "uva") color_fr = ColorRGB(255, 0, 255);
            else if (tipo == "naranja") color_fr = ColorRGB(255, 165, 0);
            
            FillRectColor(hdc, panelX, currentY, 12, 12, color_fr);
            
            // Texto con nombre, puntos y contador
            sprintf(buf, " %s: %d pts x%d", tipo.c_str(), pts, contador);
            TextOutA(hdc, panelX + 15, currentY, buf, (int)strlen(buf));
            currentY += lineHeight;
        }
        
        currentY += 10;
        
        // Controles
        TextOutA(hdc, panelX, currentY, "Controles:", 10);
        currentY += lineHeight;
        TextOutA(hdc, panelX, currentY, "W/A/S/D - Mover", 16);
        currentY += lineHeight;
        TextOutA(hdc, panelX, currentY, "P - Pausa", 9);
        currentY += lineHeight;
        TextOutA(hdc, panelX, currentY, "ESC - Salir", 11);
        currentY += lineHeight;
        
        // Mensaje de game over
        if (game_over) {
            currentY += 10;
            SetTextColor(hdc, RGB(255, 100, 100));
            TextOutA(hdc, panelX, currentY, "GAME OVER!", 10);
            currentY += lineHeight;
            SetTextColor(hdc, RGB(255, 255, 255));
            TextOutA(hdc, panelX, currentY, "Presiona R para", 15);
            currentY += lineHeight;
            TextOutA(hdc, panelX, currentY, "reiniciar", 9);
        }
        
        // Mensaje de pausa
        if (pausado && !game_over) {
            currentY += 10;
            SetTextColor(hdc, RGB(255, 255, 100));
            TextOutA(hdc, panelX, currentY, "PAUSA", 5);
            currentY += lineHeight;
            SetTextColor(hdc, RGB(255, 255, 255));
            TextOutA(hdc, panelX, currentY, "Presiona P para", 15);
            currentY += lineHeight;
            TextOutA(hdc, panelX, currentY, "continuar", 9);
        }
        
        // Restaurar color de texto
        SetTextColor(hdc, RGB(255, 255, 255));
    }
    /**
     * Ejecuta el bucle principal del juego.
     * Continúa ejecutándose incluso cuando hay game over para permitir reiniciar.
     */
    void run() {
        HDC hdcWindow = GetDC(g_hWnd);
        HDC memDC = CreateCompatibleDC(hdcWindow);
        HBITMAP hbm = CreateCompatibleBitmap(hdcWindow, 900, 700);
        HBITMAP oldbm = (HBITMAP)SelectObject(memDC, hbm);
        
        while (juego_activo && g_running) {
            // Procesar mensajes de Windows
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    juego_activo = false;
                    g_running = false;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            
            // Procesar entrada (incluye reinicio si hay game over)
            procesarTeclas();
            
            // Actualizar física solo si no está pausado ni en game over
            if (!pausado && !game_over) {
                actualizarFisica();
            }
            
            // Renderizar siempre (muestra game over si es necesario)
            renderizar(memDC);
            
            // Copiar buffer a la ventana
            BitBlt(hdcWindow, 0, 0, 900, 700, memDC, 0, 0, SRCCOPY);
            
            // Pequeña pausa para no saturar la CPU (~60 FPS)
            Sleep(16);
        }
        
        // Limpiar recursos de GDI
        SelectObject(memDC, oldbm);
        DeleteObject(hbm);
        DeleteDC(memDC);
        ReleaseDC(g_hWnd, hdcWindow);
    }
};

#endif // USE_GDI

// ================================================================
// Función para seleccionar modo de renderizado
// ================================================================
ModoRenderizado seleccionarModo() {
#ifdef USE_GDI
    cout << "\n============================================\n";
    cout << "        SELECCION DE MODO DE RENDERIZADO\n";
    cout << "============================================\n";
    cout << "1) Modo Consola (texto en terminal)\n";
    cout << "2) Modo Ventana Grafica (GDI - Windows)\n";
    cout << "\nElige una opcion (1-2): ";
    int modo = 0;
    cin >> modo;
    if (modo == 2) {
        cout << "Modo seleccionado: Ventana Grafica (GDI)\n";
        return MODO_VENTANA;
    } else {
        cout << "Modo seleccionado: Consola\n";
        return MODO_CONSOLA;
    }
#else
    cout << "\n[INFO] Modo grafico no disponible (compilar con -DUSE_GDI)\n";
    cout << "[INFO] Usando modo consola por defecto.\n";
    return MODO_CONSOLA;
#endif
}

int main(){
    cout << "============================================\n";
    cout << "           RUNTIME - SELECCION DE JUEGO\n";
    cout << "============================================\n";
    cout << "1) Tetris\n2) Snake\n3) Salir\n";
    cout << "Elige una opcion (1-3): ";
    int op = 0;
    cin >> op;

    if (op < 1 || op > 2) {
        cout << "Saliendo...\n";
        return 0;
    }

    modoActual = seleccionarModo();
    cout << "\n";

    switch(op){
        case 1: {
            cout << "Compilando configuracion de Tetris y lanzando juego...\n";
            compilarJuegoSiPosible("tetris");
            // Ejecutar la versión gráfica adecuada si se seleccionó MODO_VENTANA
#ifdef USE_GDI
            if (modoActual == MODO_VENTANA) {
                // Build a temporary config to size window and print debug info
                ConfigTetris cfg;
                cfg.printConfig();
                // desired size based on AST
                int desired_w = cfg.ancho_tablero * cfg.tamanio_celda + 200;
                int desired_h = cfg.alto_tablero * cfg.tamanio_celda + 200;
                // clamp to screen size, scale cell if needed
                int screenW = GetSystemMetrics(SM_CXSCREEN);
                int screenH = GetSystemMetrics(SM_CYSCREEN);
                int forced_cell = cfg.tamanio_celda;
                if (desired_w > screenW - 100 || desired_h > screenH - 100) {
                    double scaleW = (double)(screenW - 200) / (double)(cfg.ancho_tablero * cfg.tamanio_celda);
                    double scaleH = (double)(screenH - 200) / (double)(cfg.alto_tablero * cfg.tamanio_celda);
                    double scale = (scaleW < scaleH) ? scaleW : scaleH;
                    if (scale <= 0) scale = 0.5;
                    forced_cell = (int)(cfg.tamanio_celda * scale);
                    if (forced_cell < 8) forced_cell = 8;
                    desired_w = cfg.ancho_tablero * forced_cell + 200;
                    desired_h = cfg.alto_tablero * forced_cell + 200;
                    std::cout << "[Tetris GDI] Window would overflow screen; scaling cell from " << cfg.tamanio_celda << " to " << forced_cell << std::endl;
                }
                if (createGDIWindow("Tetris - Motor de Ladrillos (GDI)", desired_w, desired_h)) {
                    TetrisEngineGDI engine(forced_cell);
                    std::cout << "[Tetris GDI] Starting with window " << desired_w << "x" << desired_h << ", cell=" << engine.cell << std::endl;
                    engine.run();
                } else {
                    // Fallback a consola
                    TetrisEngine t;
                    t.ejecutar();
                }
            } else {
                TetrisEngine t;
                t.ejecutar();
            }
#else
            {
                TetrisEngine t;
                t.ejecutar();
            }
#endif
            cout << "\nGracias por jugar Tetris!\n";
            break;
        }
        case 2: {
            cout << "Compilando configuracion de Snake y lanzando juego...\n";
            compilarJuegoSiPosible("snake");
            // Ejecutar la versión gráfica adecuada si se seleccionó MODO_VENTANA
#ifdef USE_GDI
            if (modoActual == MODO_VENTANA) {
                if (createGDIWindow("Snake - Motor de Ladrillos (GDI)", 900, 700)) {
                    SnakeEngineGDI engine;
                    engine.run();
                } else {
                    SnakeEngine s;
                    s.ejecutar();
                }
            } else {
                SnakeEngine s;
                s.ejecutar();
            }
#else
            {
                SnakeEngine s;
                s.ejecutar();
            }
#endif
            cout << "\nGracias por jugar Snake!\n";
            break;
        }
        default:
            cout << "Saliendo...\n";
    }
    return 0;
}


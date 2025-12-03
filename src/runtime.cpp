// Runtime unificado: permite elegir Tetris o Snake en un solo binario
// Mantiene parsers AST simples y motores de juego con renderizado sin parpadeo.
// Soporta modo CONSOLA (por defecto) y modo VENTANA GRAFICA (SDL2)

// Define USE_SDL para habilitar el modo gráfico con SDL2
// Compilar con: g++ -DUSE_SDL -o runtime runtime.cpp -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf
// Sin SDL (solo consola): g++ -o runtime runtime.cpp

#include <algorithm>
#include <ctime>
#include <conio.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#ifdef USE_SDL
// Note: avoid <random>/<chrono> to keep C++98 compatibility on older toolchains
#endif
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

#ifdef USE_SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#endif

using namespace std;
// usando GetTickCount + <ctime> para compatibilidad C++03/Windows XP

// Variable global para el modo de renderizado
enum ModoRenderizado { MODO_CONSOLA = 1, MODO_VENTANA = 2 };
ModoRenderizado modoActual = MODO_CONSOLA;


class ColorConsola {
private:
    HANDLE hConsole;
    WORD   color_original;

public:
    ColorConsola() {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(hConsole, &info);
        color_original = info.wAttributes;
    }
    ~ColorConsola() { restaurarColor(); }

    void establecerColor(int codigo_color) {
        SetConsoleTextAttribute(hConsole, codigo_color);
    }

    void restaurarColor() {
        SetConsoleTextAttribute(hConsole, color_original);
    }

    // Mapeo por nombre para Snake
    string obtenerColorAnsi(const string& nombre_color) {
        if (nombre_color == "verde_claro" || nombre_color == "verde") return "\033[92m";
        if (nombre_color == "verde_oscuro") return "\033[32m";
        if (nombre_color == "verde_medio") return "\033[36m";
        if (nombre_color == "rojo") return "\033[91m";
        if (nombre_color == "amarillo") return "\033[93m";
        if (nombre_color == "blanco") return "\033[97m";
        if (nombre_color == "gris") return "\033[90m";
        if (nombre_color == "negro") return "\033[30m";
        return "\033[37m"; // Blanco por defecto
    }
};

// Singleton compatible con el engine de 

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
    map<string, int> colores;
    map<string, vector<vector<vector<int> > > > rotaciones_piezas;
    vector<string> tipos_piezas;
    string nombre_juego;
    int ancho_tablero;
    int alto_tablero;
    int velocidad_inicial;

    ConfigTetris() {
        cargarDesdeAST();
        configurarRotacionesHardcoded();
    }
private:
    void cargarDesdeAST() {
        ASTParser parser;
        if (!parser.cargarDesdeAST("build/arbol.ast")) {
            nombre_juego = "Tetris Clásico";
            ancho_tablero = 10;
            alto_tablero = 20;
            velocidad_inicial = 800;
            return;
        }
        nombre_juego = parser.obtenerString("nombre_juego", "Tetris Clásico");
        ancho_tablero = parser.obtenerInt("ancho_tablero", 10);
        alto_tablero = parser.obtenerInt("alto_tablero", 20);
        velocidad_inicial = parser.obtenerInt("velocidad_inicial", 800);
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
                if (parser.objects_int["codigos_color"].count(color_nombre)) {
                    colores[pieza] = parser.objects_int["codigos_color"][color_nombre];
                }
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
    int velocidad_caida;

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

            int nuevo_nivel = (lineas_completadas / 10) + 1;
            if (nuevo_nivel > nivel) {
                nivel = nuevo_nivel;
                velocidad_caida = max(100, 800 - (nivel * 50));
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
        switch (n) {
            case 1: puntos += 100 * nivel; break;
            case 2: puntos += 300 * nivel; break;
            case 3: puntos += 500 * nivel; break;
            case 4: puntos += 800 * nivel; break; // TETRIS
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
            case 'a':
                if (esMovimientoValido(pieza_actual->x - 1, pieza_actual->y, pieza_actual->rotacion_actual)) pieza_actual->x--;
                break;
            case 'd':
                if (esMovimientoValido(pieza_actual->x + 1, pieza_actual->y, pieza_actual->rotacion_actual)) pieza_actual->x++;
                break;
            case 's':
                while (esMovimientoValido(pieza_actual->x, pieza_actual->y + 1, pieza_actual->rotacion_actual)) {
                    pieza_actual->y++;
                    puntos += 1;
                }
                fijarPieza();
                break;
            case 'w':
            case ' ': {
                int nr = (pieza_actual->rotacion_actual + 1) % static_cast<int>(pieza_actual->rotaciones.size());
                if (esMovimientoValido(pieza_actual->x, pieza_actual->y, nr)) {
                    pieza_actual->rotacion_actual = nr;
                    //puntos += 10;
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
        nivel             = 1;
        lineas_completadas = 0;
        velocidad_caida   = 800;
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
    bool               juego_activo;
    bool               pausado;
    bool               game_over;
    int                puntos;
    int                nivel;
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
         nivel(1) {
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
        generarNuevaFruta();
        puntos    = 0;
        game_over = false;
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
    }

    bool esPosicionOcupadaPorSnake(const Posicion& p) {
        for (size_t i = 0; i < cuerpo_snake.size(); ++i) {
            if (cuerpo_snake[i] == p) return true;
        }
        return false;
    }

    bool hayColisionConCuerpo(const Posicion& p) {
        for (size_t i = 1; i < cuerpo_snake.size(); ++i) {
            if (cuerpo_snake[i] == p) return true;
        }
        return false;
    }

    bool estaFueraDelTablero(const Posicion& p) {
        return p.x <= 0 || p.x >= ancho_tablero - 1 || p.y <= 0 || p.y >= alto_tablero - 1;
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
        Posicion nueva(cuerpo_snake[0].x + direccion_actual.x,
                       cuerpo_snake[0].y + direccion_actual.y);

        bool fin_borde = config.booleans.count("terminar_al_chocar_borde")
                        ? config.booleans["terminar_al_chocar_borde"]
                        : true;
        bool fin_cuerpo = config.booleans.count("terminar_al_chocar_cuerpo")
                        ? config.booleans["terminar_al_chocar_cuerpo"]
                        : true;

        if (fin_borde && estaFueraDelTablero(nueva)) {
            game_over = true;
            return;
        }
        if (fin_cuerpo && hayColisionConCuerpo(nueva)) {
            game_over = true;
            return;
        }

        cuerpo_snake.insert(cuerpo_snake.begin(), nueva);
        if (nueva == fruta_posicion) {
            int pf = config.integers.count("puntos_por_fruta")
                   ? config.integers["puntos_por_fruta"]
                   : 100;
            puntos += pf;
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
                    string cf = config.strings.count("color_fruta")
                              ? config.strings["color_fruta"]
                              : string("rojo");
                    ch  = '@';
                    col = colorConsola.obtenerColorAnsi(cf);
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

// ================================================================
// Runtime selector
// ================================================================
static void compilarJuegoSiPosible(const string& juego){
    // Intenta invocar el compilador con el .brik correspondiente (opcional)
    string comando;
    if (juego == "tetris")      comando = "bin\\compilador.exe config\\games\\Tetris.brik";
    else if (juego == "snake")  comando = "bin\\compilador.exe config\\games\\Snake.brik";
    else return;
    // Ejecutar de forma silenciosa si existe el compilador
    DWORD attrs = GetFileAttributesA("bin\\compilador.exe");
    if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        system(comando.c_str());
    }
}

// ================================================================
// SDL2 - Motores gráficos (solo disponible si se compila con USE_SDL)
// ================================================================
#ifdef USE_SDL

// Colores SDL para las piezas de Tetris
SDL_Color obtenerColorSDL(int codigo) {
    switch (codigo) {
        case 9:  return {0, 100, 255, 255};   // Azul
        case 10: return {0, 255, 100, 255};   // Verde
        case 11: return {0, 255, 255, 255};   // Cian
        case 12: return {255, 100, 0, 255};   // Naranja/Rojo
        case 13: return {200, 0, 200, 255};   // Magenta
        case 14: return {255, 255, 0, 255};   // Amarillo
        case 15: return {255, 255, 255, 255}; // Blanco
        default: return {128, 128, 128, 255}; // Gris
    }
}

SDL_Color obtenerColorSnakeSDL(const string& nombre) {
    if (nombre == "verde_claro" || nombre == "verde") return {50, 255, 50, 255};
    if (nombre == "verde_oscuro") return {0, 150, 0, 255};
    if (nombre == "verde_medio") return {0, 200, 100, 255};
    if (nombre == "rojo") return {255, 50, 50, 255};
    if (nombre == "amarillo") return {255, 255, 0, 255};
    if (nombre == "blanco") return {255, 255, 255, 255};
    if (nombre == "gris") return {128, 128, 128, 255};
    return {255, 255, 255, 255};
}

// ================================================================
// TetrisEngineSDL - Versión gráfica de Tetris
// ================================================================
class TetrisEngineSDL {
private:
    static const int ANCHO = 10;
    static const int ALTO  = 20;
    static const int CELL_SIZE = 30;
    static const int OFFSET_X = 50;
    static const int OFFSET_Y = 50;

    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;

    vector<vector<TipoPieza>>   tablero;
    vector<vector<ColorTetris>> colores_tablero;

    PiezaTetris* pieza_actual;
    PiezaTetris* siguiente_pieza;

    int puntos;
    int nivel;
    int lineas_completadas;
    int velocidad_caida;

    bool juego_activo;
    bool pausado;
    bool game_over;

    DWORD ultima_caida;
    ConfigTetris config;

public:
    TetrisEngineSDL()
        : tablero(ALTO, vector<TipoPieza>(ANCHO, VACIO))
        , colores_tablero(ALTO, vector<ColorTetris>(ANCHO, GRIS))
        , pieza_actual(NULL)
        , siguiente_pieza(NULL)
        , puntos(0)
        , nivel(1)
        , lineas_completadas(0)
        , velocidad_caida(800)
        , juego_activo(true)
        , pausado(false)
        , game_over(false)
        , window(NULL)
        , renderer(NULL)
        , font(NULL) {
        
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            cerr << "Error SDL: " << SDL_GetError() << endl;
            juego_activo = false;
            return;
        }
        
        if (TTF_Init() < 0) {
            cerr << "Error SDL_ttf: " << TTF_GetError() << endl;
        }

        window = SDL_CreateWindow("Tetris - Motor de Ladrillos",
                                  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  600, 700, SDL_WINDOW_SHOWN);
        if (!window) {
            cerr << "Error creando ventana: " << SDL_GetError() << endl;
            juego_activo = false;
            return;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            cerr << "Error creando renderer: " << SDL_GetError() << endl;
            juego_activo = false;
            return;
        }

        // Intentar cargar una fuente del sistema
        font = TTF_OpenFont("C:\\Windows\\Fonts\\consola.ttf", 18);
        if (!font) font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 18);

        // Inicializar semilla y temporizador en ms (GetTickCount)
        srand((unsigned)time(NULL));
        ultima_caida = GetTickCount();
        generarNuevaPieza();
        generarSiguientePieza();
    }

    ~TetrisEngineSDL() {
        delete pieza_actual;
        delete siguiente_pieza;
        if (font) TTF_CloseFont(font);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
    }

    void generarNuevaPieza() {
        if (siguiente_pieza) {
            delete pieza_actual;
            pieza_actual = siguiente_pieza;
            siguiente_pieza = NULL;
        } else {
            pieza_actual = new PiezaTetris(static_cast<TipoPieza>(rand() % 7), config);
        }
        pieza_actual->x = ANCHO / 2 - 2;
        pieza_actual->y = 0;
        pieza_actual->rotacion_actual = 0;

        if (!esMovimientoValido(pieza_actual->x, pieza_actual->y, pieza_actual->rotacion_actual)) {
            game_over = true;
        }
    }

    void generarSiguientePieza() {
        siguiente_pieza = new PiezaTetris(static_cast<TipoPieza>(rand() % 7), config);
    }

    bool esMovimientoValido(int nx, int ny, int nr) {
        if (!pieza_actual) return false;
        vector<vector<int> >& forma = pieza_actual->rotaciones[nr];
        int h = (int)forma.size();
        int w = (int)forma[0].size();

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
        int h = (int)forma.size();
        int w = (int)forma[0].size();

        for (int py = 0; py < h; ++py) {
            for (int px = 0; px < w; ++px) {
                if (forma[py][px] == 1) {
                    int wx = pieza_actual->x + px;
                    int wy = pieza_actual->y + py;
                    if (wy >= 0 && wx >= 0 && wx < ANCHO && wy < ALTO) {
                        tablero[wy][wx] = pieza_actual->tipo;
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
            if (completa) lineas.push_back(y);
        }

        if (!lineas.empty()) {
            eliminarLineas(lineas);
            calcularPuntos((int)lineas.size());
            lineas_completadas += (int)lineas.size();

            int nuevo_nivel = (lineas_completadas / 10) + 1;
            if (nuevo_nivel > nivel) {
                nivel = nuevo_nivel;
                velocidad_caida = max(100, 800 - (nivel * 50));
            }
        }
    }

    void eliminarLineas(const vector<int>& lineas) {
        for (int i = (int)lineas.size() - 1; i >= 0; --i) {
            int linea = lineas[i];
            for (int y = linea; y > 0; --y) {
                for (int x = 0; x < ANCHO; ++x) {
                    tablero[y][x] = tablero[y - 1][x];
                    colores_tablero[y][x] = colores_tablero[y - 1][x];
                }
            }
            for (int x = 0; x < ANCHO; ++x) {
                tablero[0][x] = VACIO;
                colores_tablero[0][x] = GRIS;
            }
        }
    }

    void calcularPuntos(int n) {
        switch (n) {
            case 1: puntos += 100 * nivel; break;
            case 2: puntos += 300 * nivel; break;
            case 3: puntos += 500 * nivel; break;
            case 4: puntos += 800 * nivel; break;
        }
    }

    void procesarEntrada() {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                juego_activo = false;
            } else if (e.type == SDL_KEYDOWN) {
                if (game_over) {
                    if (e.key.keysym.sym == SDLK_r) reiniciarJuego();
                    continue;
                }
                switch (e.key.keysym.sym) {
                    case SDLK_a:
                    case SDLK_LEFT:
                        if (esMovimientoValido(pieza_actual->x - 1, pieza_actual->y, pieza_actual->rotacion_actual))
                            pieza_actual->x--;
                        break;
                    case SDLK_d:
                    case SDLK_RIGHT:
                        if (esMovimientoValido(pieza_actual->x + 1, pieza_actual->y, pieza_actual->rotacion_actual))
                            pieza_actual->x++;
                        break;
                    case SDLK_s:
                    case SDLK_DOWN:
                        while (esMovimientoValido(pieza_actual->x, pieza_actual->y + 1, pieza_actual->rotacion_actual)) {
                            pieza_actual->y++;
                            puntos += 1;
                        }
                        fijarPieza();
                        break;
                    case SDLK_w:
                    case SDLK_UP:
                    case SDLK_SPACE: {
                        int nr = (pieza_actual->rotacion_actual + 1) % (int)pieza_actual->rotaciones.size();
                        if (esMovimientoValido(pieza_actual->x, pieza_actual->y, nr))
                            pieza_actual->rotacion_actual = nr;
                        break;
                    }
                    case SDLK_p:
                        pausado = !pausado;
                        break;
                    case SDLK_ESCAPE:
                        juego_activo = false;
                        break;
                }
            }
        }
    }

    void reiniciarJuego() {
        for (int y = 0; y < ALTO; ++y) {
            for (int x = 0; x < ANCHO; ++x) {
                tablero[y][x] = VACIO;
                colores_tablero[y][x] = GRIS;
            }
        }
        puntos = 0;
        nivel = 1;
        lineas_completadas = 0;
        velocidad_caida = 800;
        game_over = false;
        juego_activo = true;
        pausado = false;

        delete pieza_actual;
        delete siguiente_pieza;
        pieza_actual = NULL;
        siguiente_pieza = NULL;

        generarNuevaPieza();
        generarSiguientePieza();
        ultima_caida = GetTickCount();
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

    void renderizarTexto(const string& texto, int x, int y, SDL_Color color) {
        if (!font) return;
        SDL_Surface* surface = TTF_RenderText_Solid(font, texto.c_str(), color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                SDL_Rect dest = {x, y, surface->w, surface->h};
                SDL_RenderCopy(renderer, texture, NULL, &dest);
                SDL_DestroyTexture(texture);
            }
            SDL_FreeSurface(surface);
        }
    }

    void renderizar() {
        SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
        SDL_RenderClear(renderer);

        // Dibujar marco del tablero
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_Rect marco = {OFFSET_X - 2, OFFSET_Y - 2, ANCHO * CELL_SIZE + 4, ALTO * CELL_SIZE + 4};
        SDL_RenderDrawRect(renderer, &marco);

        // Dibujar tablero y pieza actual
        for (int y = 0; y < ALTO; ++y) {
            for (int x = 0; x < ANCHO; ++x) {
                SDL_Rect cell = {OFFSET_X + x * CELL_SIZE, OFFSET_Y + y * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1};
                
                bool es_pieza = false;
                SDL_Color col = {30, 30, 40, 255};

                // Verificar si es parte de la pieza actual
                if (pieza_actual && !pausado && !game_over) {
                    vector<vector<int> >& f = pieza_actual->rotaciones[pieza_actual->rotacion_actual];
                    int py = y - pieza_actual->y;
                    int px = x - pieza_actual->x;
                    if (py >= 0 && py < (int)f.size() && px >= 0 && px < (int)f[0].size() && f[py][px] == 1) {
                        col = obtenerColorSDL(static_cast<int>(pieza_actual->color));
                        es_pieza = true;
                    }
                }

                // Si no es pieza actual, verificar tablero fijo
                if (!es_pieza && tablero[y][x] != VACIO) {
                    col = obtenerColorSDL(static_cast<int>(colores_tablero[y][x]));
                }

                SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
                SDL_RenderFillRect(renderer, &cell);
            }
        }

        // Panel de información
        SDL_Color blanco = {255, 255, 255, 255};
        SDL_Color amarillo = {255, 255, 0, 255};
        
        int infoX = OFFSET_X + ANCHO * CELL_SIZE + 30;
        renderizarTexto("=== TETRIS ===", infoX, 50, blanco);
        {
            std::ostringstream oss; oss << "Puntos: " << puntos;
            renderizarTexto(oss.str(), infoX, 90, amarillo);
        }
        {
            std::ostringstream oss; oss << "Nivel: " << nivel;
            renderizarTexto(oss.str(), infoX, 120, amarillo);
        }
        {
            std::ostringstream oss; oss << "Lineas: " << lineas_completadas;
            renderizarTexto(oss.str(), infoX, 150, amarillo);
        }
        
        renderizarTexto("Controles:", infoX, 200, blanco);
        renderizarTexto("A/D o Flechas: Mover", infoX, 230, {150, 150, 150, 255});
        renderizarTexto("W/Space: Rotar", infoX, 260, {150, 150, 150, 255});
        renderizarTexto("S: Caida rapida", infoX, 290, {150, 150, 150, 255});
        renderizarTexto("P: Pausa", infoX, 320, {150, 150, 150, 255});
        renderizarTexto("ESC: Salir", infoX, 350, {150, 150, 150, 255});

        // Siguiente pieza
        if (siguiente_pieza) {
            renderizarTexto("Siguiente:", infoX, 400, blanco);
            vector<vector<int> >& f = siguiente_pieza->rotaciones[0];
            SDL_Color col = obtenerColorSDL(static_cast<int>(siguiente_pieza->color));
            for (int py = 0; py < (int)f.size(); ++py) {
                for (int px = 0; px < (int)f[0].size(); ++px) {
                    if (f[py][px] == 1) {
                        SDL_Rect cell = {infoX + px * 20, 430 + py * 20, 18, 18};
                        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
                        SDL_RenderFillRect(renderer, &cell);
                    }
                }
            }
        }

        if (game_over) {
            SDL_Color rojo = {255, 50, 50, 255};
            renderizarTexto("!GAME OVER!", OFFSET_X + 80, OFFSET_Y + ALTO * CELL_SIZE / 2 - 30, rojo);
            renderizarTexto("Presiona R para reiniciar", OFFSET_X + 40, OFFSET_Y + ALTO * CELL_SIZE / 2, blanco);
        }

        if (pausado) {
            SDL_Color magenta = {200, 50, 200, 255};
            renderizarTexto("*** PAUSADO ***", OFFSET_X + 70, OFFSET_Y + ALTO * CELL_SIZE / 2, magenta);
        }

        SDL_RenderPresent(renderer);
    }

    void ejecutar() {
        while (juego_activo) {
            procesarEntrada();
            if (!pausado && !game_over) {
                actualizarFisica();
            }
            renderizar();
            SDL_Delay(16); // ~60 FPS
        }
    }
};

// ================================================================
// SnakeEngineSDL - Versión gráfica de Snake
// ================================================================
class SnakeEngineSDL {
private:
    static const int CELL_SIZE = 20;
    static const int OFFSET_X = 50;
    static const int OFFSET_Y = 80;

    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;

    SnakeAST config;
    vector<Posicion> cuerpo_snake;
    Posicion direccion_actual;
    Posicion fruta_posicion;
    
    bool juego_activo;
    bool pausado;
    bool game_over;
    int puntos;
    int nivel;
    
    // using rand() for C++03 compatibility
    int ancho_tablero;
    int alto_tablero;
    int velocidad_ms;

    DWORD ultima_actualizacion;

public:
    SnakeEngineSDL()
        : juego_activo(true)
        , pausado(false)
        , game_over(false)
        , puntos(0)
        , nivel(1)
        , window(NULL)
        , renderer(NULL)
        , font(NULL) {
        
        cargarConfiguracionAST();

        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            cerr << "Error SDL: " << SDL_GetError() << endl;
            juego_activo = false;
            return;
        }
        
        if (TTF_Init() < 0) {
            cerr << "Error SDL_ttf: " << TTF_GetError() << endl;
        }

        int windowW = ancho_tablero * CELL_SIZE + 100 + 200;
        int windowH = alto_tablero * CELL_SIZE + 160;

        window = SDL_CreateWindow("Snake - Motor de Ladrillos",
                                  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  windowW, windowH, SDL_WINDOW_SHOWN);
        if (!window) {
            cerr << "Error creando ventana: " << SDL_GetError() << endl;
            juego_activo = false;
            return;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            cerr << "Error creando renderer: " << SDL_GetError() << endl;
            juego_activo = false;
            return;
        }

        font = TTF_OpenFont("C:\\Windows\\Fonts\\consola.ttf", 16);
        if (!font) font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 16);

        srand((unsigned)time(NULL));
        inicializarJuego();
        ultima_actualizacion = GetTickCount();
    }

    ~SnakeEngineSDL() {
        if (font) TTF_CloseFont(font);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
    }

private:
    void cargarConfiguracionAST() {
        if (!config.cargarDesdeAST("build/arbol.ast")) {
            ancho_tablero = 25;
            alto_tablero = 20;
            velocidad_ms = 150;
            return;
        }
        ancho_tablero = config.integers.count("ancho_tablero") ? config.integers["ancho_tablero"] : 25;
        alto_tablero = config.integers.count("alto_tablero") ? config.integers["alto_tablero"] : 20;
        velocidad_ms = config.integers.count("velocidad_inicial") ? config.integers["velocidad_inicial"] : 150;
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
        generarNuevaFruta();
        puntos = 0;
        game_over = false;
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
    }

    bool esPosicionOcupadaPorSnake(const Posicion& p) {
        for (size_t i = 0; i < cuerpo_snake.size(); ++i) {
            if (cuerpo_snake[i] == p) return true;
        }
        return false;
    }

    bool hayColisionConCuerpo(const Posicion& p) {
        for (size_t i = 1; i < cuerpo_snake.size(); ++i) {
            if (cuerpo_snake[i] == p) return true;
        }
        return false;
    }

    bool estaFueraDelTablero(const Posicion& p) {
        return p.x <= 0 || p.x >= ancho_tablero - 1 || p.y <= 0 || p.y >= alto_tablero - 1;
    }

    void procesarEntrada() {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                juego_activo = false;
            } else if (e.type == SDL_KEYDOWN) {
                if (game_over) {
                    if (e.key.keysym.sym == SDLK_r) {
                        inicializarJuego();
                        ultima_actualizacion = GetTickCount();
                    } else if (e.key.keysym.sym == SDLK_ESCAPE) {
                        juego_activo = false;
                    }
                    continue;
                }
                switch (e.key.keysym.sym) {
                    case SDLK_w: case SDLK_UP:
                        if (direccion_actual.y == 0) direccion_actual = Posicion(0, -1);
                        break;
                    case SDLK_s: case SDLK_DOWN:
                        if (direccion_actual.y == 0) direccion_actual = Posicion(0, 1);
                        break;
                    case SDLK_a: case SDLK_LEFT:
                        if (direccion_actual.x == 0) direccion_actual = Posicion(-1, 0);
                        break;
                    case SDLK_d: case SDLK_RIGHT:
                        if (direccion_actual.x == 0) direccion_actual = Posicion(1, 0);
                        break;
                    case SDLK_p:
                        pausado = !pausado;
                        break;
                    case SDLK_ESCAPE:
                        juego_activo = false;
                        break;
                }
            }
        }
    }

    void actualizarFisica() {
        if (pausado || game_over) return;

        DWORD ahora = GetTickCount();
        DWORD dt = ahora - ultima_actualizacion;

        if (dt < (DWORD)velocidad_ms) return;
        ultima_actualizacion = ahora;

        Posicion nueva(cuerpo_snake[0].x + direccion_actual.x,
                       cuerpo_snake[0].y + direccion_actual.y);

        bool fin_borde = config.booleans.count("terminar_al_chocar_borde") ? config.booleans["terminar_al_chocar_borde"] : true;
        bool fin_cuerpo = config.booleans.count("terminar_al_chocar_cuerpo") ? config.booleans["terminar_al_chocar_cuerpo"] : true;

        if (fin_borde && estaFueraDelTablero(nueva)) {
            game_over = true;
            return;
        }
        if (fin_cuerpo && hayColisionConCuerpo(nueva)) {
            game_over = true;
            return;
        }

        cuerpo_snake.insert(cuerpo_snake.begin(), nueva);
        if (nueva == fruta_posicion) {
            int pf = config.integers.count("puntos_por_fruta") ? config.integers["puntos_por_fruta"] : 100;
            puntos += pf;
            generarNuevaFruta();
        } else {
            cuerpo_snake.pop_back();
        }
    }

    void renderizarTexto(const string& texto, int x, int y, SDL_Color color) {
        if (!font) return;
        SDL_Surface* surface = TTF_RenderText_Solid(font, texto.c_str(), color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                SDL_Rect dest = {x, y, surface->w, surface->h};
                SDL_RenderCopy(renderer, texture, NULL, &dest);
                SDL_DestroyTexture(texture);
            }
            SDL_FreeSurface(surface);
        }
    }

    void renderizar() {
        SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
        SDL_RenderClear(renderer);

        // Título e información
        SDL_Color blanco = {255, 255, 255, 255};
        SDL_Color amarillo = {255, 255, 0, 255};
        
        string titulo = config.strings.count("nombre_juego") ? config.strings["nombre_juego"] : "Snake Clasico";
        renderizarTexto("=== " + titulo + " ===", OFFSET_X, 15, blanco);
        {
            std::ostringstream _oss; _oss << "Puntos: " << puntos << "  |  Longitud: " << cuerpo_snake.size();
            renderizarTexto(_oss.str(), OFFSET_X, 45, amarillo);
        }

        // Dibujar marco del tablero
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_Rect marco = {OFFSET_X - 2, OFFSET_Y - 2, ancho_tablero * CELL_SIZE + 4, alto_tablero * CELL_SIZE + 4};
        SDL_RenderDrawRect(renderer, &marco);

        // Dibujar fondo del tablero
        SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
        SDL_Rect fondo = {OFFSET_X, OFFSET_Y, ancho_tablero * CELL_SIZE, alto_tablero * CELL_SIZE};
        SDL_RenderFillRect(renderer, &fondo);

        // Dibujar bordes
        SDL_SetRenderDrawColor(renderer, 80, 80, 100, 255);
        for (int x = 0; x < ancho_tablero; ++x) {
            SDL_Rect top = {OFFSET_X + x * CELL_SIZE, OFFSET_Y, CELL_SIZE - 1, CELL_SIZE - 1};
            SDL_Rect bottom = {OFFSET_X + x * CELL_SIZE, OFFSET_Y + (alto_tablero - 1) * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1};
            SDL_RenderFillRect(renderer, &top);
            SDL_RenderFillRect(renderer, &bottom);
        }
        for (int y = 0; y < alto_tablero; ++y) {
            SDL_Rect left = {OFFSET_X, OFFSET_Y + y * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1};
            SDL_Rect right = {OFFSET_X + (ancho_tablero - 1) * CELL_SIZE, OFFSET_Y + y * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1};
            SDL_RenderFillRect(renderer, &left);
            SDL_RenderFillRect(renderer, &right);
        }

        // Dibujar fruta
        string colorFruta = config.strings.count("color_fruta") ? config.strings["color_fruta"] : "rojo";
        SDL_Color colFruta = obtenerColorSnakeSDL(colorFruta);
        SDL_SetRenderDrawColor(renderer, colFruta.r, colFruta.g, colFruta.b, colFruta.a);
        SDL_Rect frutaRect = {OFFSET_X + fruta_posicion.x * CELL_SIZE + 2, OFFSET_Y + fruta_posicion.y * CELL_SIZE + 2, CELL_SIZE - 4, CELL_SIZE - 4};
        SDL_RenderFillRect(renderer, &frutaRect);

        // Dibujar snake
        vector<string> colores;
        if (config.arrays.count("colores_snake")) {
            colores = config.arrays["colores_snake"];
        } else {
            colores.push_back("verde_claro"); colores.push_back("verde_oscuro");
        }
        for (size_t i = 0; i < cuerpo_snake.size(); ++i) {
            SDL_Color col;
            if (i == 0 && !colores.empty()) {
                col = obtenerColorSnakeSDL(colores[0]);
            } else if (colores.size() > 1) {
                col = obtenerColorSnakeSDL(colores[1]);
            } else {
                col = {0, 200, 0, 255};
            }
            SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
            SDL_Rect snakeRect = {OFFSET_X + cuerpo_snake[i].x * CELL_SIZE + 1, OFFSET_Y + cuerpo_snake[i].y * CELL_SIZE + 1, CELL_SIZE - 2, CELL_SIZE - 2};
            SDL_RenderFillRect(renderer, &snakeRect);
        }

        // Panel de controles
        int infoX = OFFSET_X + ancho_tablero * CELL_SIZE + 20;
        renderizarTexto("Controles:", infoX, OFFSET_Y, blanco);
        renderizarTexto("WASD/Flechas: Mover", infoX, OFFSET_Y + 30, {150, 150, 150, 255});
        renderizarTexto("P: Pausa", infoX, OFFSET_Y + 60, {150, 150, 150, 255});
        renderizarTexto("ESC: Salir", infoX, OFFSET_Y + 90, {150, 150, 150, 255});

        if (game_over) {
            SDL_Color rojo = {255, 50, 50, 255};
            string msg = config.strings.count("mensaje_game_over") ? config.strings["mensaje_game_over"] : "GAME OVER";
            size_t p = msg.find("{puntos}");
            if (p != string::npos) {
                std::ostringstream _oss; _oss << puntos;
                msg.replace(p, 8, _oss.str());
            }
            renderizarTexto(msg, OFFSET_X + 50, OFFSET_Y + alto_tablero * CELL_SIZE / 2 - 20, rojo);
            renderizarTexto("Presiona R para reiniciar", OFFSET_X + 30, OFFSET_Y + alto_tablero * CELL_SIZE / 2 + 10, blanco);
        }

        if (pausado) {
            SDL_Color magenta = {200, 50, 200, 255};
            renderizarTexto("*** PAUSADO ***", OFFSET_X + 60, OFFSET_Y + alto_tablero * CELL_SIZE / 2, magenta);
        }

        SDL_RenderPresent(renderer);
    }

public:
    void ejecutar() {
        while (juego_activo) {
            procesarEntrada();
            actualizarFisica();
            renderizar();
            SDL_Delay(16);
        }
    }
};

#endif // USE_SDL

// ================================================================
// Función para seleccionar modo de renderizado
// ================================================================
ModoRenderizado seleccionarModo() {
#ifdef USE_SDL
    cout << "\n============================================\n";
    cout << "        SELECCION DE MODO DE JUEGO\n";
    cout << "============================================\n";
    cout << "1) Modo Consola (texto en terminal)\n";
    cout << "2) Modo Ventana Grafica (SDL2)\n";
    cout << "Elige una opcion (1-2): ";
    int modo = 0;
    cin >> modo;
    if (modo == 2) return MODO_VENTANA;
#else
    cout << "\n[INFO] Modo grafico no disponible (compilar con -DUSE_SDL)\n";
    cout << "[INFO] Usando modo consola por defecto.\n";
#endif
    return MODO_CONSOLA;
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
#ifdef USE_SDL
            if (modoActual == MODO_VENTANA) {
                TetrisEngineSDL t;
                t.ejecutar();
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
#ifdef USE_SDL
            if (modoActual == MODO_VENTANA) {
                SnakeEngineSDL s;
                s.ejecutar();
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

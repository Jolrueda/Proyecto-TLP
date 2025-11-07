// Runtime unificado: permite elegir Tetris o Snake en un solo binario
// Mantiene parsers AST simples y motores de juego con renderizado sin parpadeo.

#include <algorithm>
#include <chrono>
#include <conio.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

using namespace std;
using namespace std::chrono;


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

// Singleton compatible con el engine de Tetris
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
    map<string, map<string, int>>       objects_int;
    map<string, map<string, string>>    objects_str;

    bool cargarDesdeAST(const string& archivo) {
        ifstream file(archivo);
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
        auto it = integers.find(clave);
        return (it != integers.end()) ? it->second : defecto;
    }

    string obtenerString(const string& clave, const string& defecto = "") {
        auto it = strings.find(clave);
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
        return stoi(num_str);
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
    map<string, vector<string>> arrays;

    bool cargarDesdeAST(const string& archivo) {
        ifstream file(archivo);
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
        return stoi(num_str);
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
    map<string, vector<vector<vector<int>>>> rotaciones_piezas;
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
        tipos_piezas = { "I", "J", "L", "O", "S", "Z", "T" };
        for (const string& pieza : tipos_piezas) {
            if (parser.objects_str["colores_piezas"].count(pieza)) {
                string color_nombre = parser.objects_str["colores_piezas"][pieza];
                if (parser.objects_int["codigos_color"].count(color_nombre)) {
                    colores[pieza] = parser.objects_int["codigos_color"][color_nombre];
                }
            }
        }
    }
    void configurarRotacionesHardcoded() {
        rotaciones_piezas["I"] = {
            {   {0,0,0,0},
                {1,1,1,1},
                {0,0,0,0},
                {0,0,0,0} },

            {   {0,1,0,0},
                {0,1,0,0},
                {0,1,0,0},
                {0,1,0,0} },

            {   {0,0,0,0},
                {1,1,1,1},
                {0,0,0,0},
                {0,0,0,0} },

            {   {0,1,0,0},
                {0,1,0,0},
                {0,1,0,0},
                {0,1,0,0} }
        };

        rotaciones_piezas["J"] = {
            {   {1,0,0},
                {1,1,1},
                {0,0,0} },

            {   {0,1,1},
                {0,1,0},
                {0,1,0} },

            {   {0,0,0},
                {1,1,1},
                {0,0,1} },

            {   {0,1,0},    
                {0,1,0},
                {1,1,0} }
        };

        rotaciones_piezas["L"] = {
            {   {0,0,1},
                {1,1,1},    
                {0,0,0} },

            {   {0,1,0},
                {0,1,0},
                {0,1,1} },

            {   {0,0,0},
                {1,1,1},
                {1,0,0} },

            {   {1,1,0},
                {0,1,0},
                {0,1,0} }
        };
        rotaciones_piezas["O"] = {
            { {1,1},{1,1} }, { {1,1},{1,1} }, { {1,1},{1,1} }, { {1,1},{1,1} }
        };
        rotaciones_piezas["S"] = {
            { {0,1,1},{1,1,0},{0,0,0} },
            { {0,1,0},{0,1,1},{0,0,1} },
            { {0,1,1},{1,1,0},{0,0,0} },
            { {0,1,0},{0,1,1},{0,0,1} }
        };
        rotaciones_piezas["Z"] = {
            { {1,1,0},{0,1,1},{0,0,0} },
            { {0,0,1},{0,1,1},{0,1,0} },
            { {1,1,0},{0,1,1},{0,0,0} },
            { {0,0,1},{0,1,1},{0,1,0} }
        };
        rotaciones_piezas["T"] = {
            { {0,1,0},{1,1,1},{0,0,0} },
            { {0,1,0},{0,1,1},{0,1,0} },
            { {0,0,0},{1,1,1},{0,1,0} },
            { {0,1,0},{1,1,0},{0,1,0} }
        };
    }
};

enum class TipoPieza { I=0,J=1,L=2,O=3,S=4,Z=5,T=6, VACIO=7 };
enum class ColorTetris { CIAN=11, AZUL=9, NARANJA=12, AMARILLO=14, VERDE=10, ROJO=12, MAGENTA=13, BLANCO=15, GRIS=8 };

class PiezaTetris {
public:
    vector<vector<vector<int>>> rotaciones;
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
            case TipoPieza::I: tipo_str = "I"; break;
            case TipoPieza::J: tipo_str = "J"; break;
            case TipoPieza::L: tipo_str = "L"; break;
            case TipoPieza::O: tipo_str = "O"; break;
            case TipoPieza::S: tipo_str = "S"; break;
            case TipoPieza::Z: tipo_str = "Z"; break;
            case TipoPieza::T: tipo_str = "T"; break;
            default:           tipo_str = "I"; break;
        }
        if (config.rotaciones_piezas.count(tipo_str)) {
            rotaciones = config.rotaciones_piezas.at(tipo_str);
        }
        if (config.colores.count(tipo_str)) {
            color = static_cast<ColorTetris>(config.colores.at(tipo_str));
        } else {
            color = ColorTetris::BLANCO;
        }
    }
};

class TetrisEngine {
private:
    static const int ANCHO = 10;
    static const int ALTO  = 20;

    vector<vector<TipoPieza>>   tablero;
    vector<vector<ColorTetris>> colores_tablero;

    PiezaTetris* pieza_actual;
    PiezaTetris* siguiente_pieza;

    mt19937 generador;
    uniform_int_distribution<int> distribucion_piezas;

    int puntos;
    int nivel;
    int lineas_completadas;
    int velocidad_caida;

    bool juego_activo;
    bool pausado;
    bool game_over;

    high_resolution_clock::time_point ultimo_movimiento;
    high_resolution_clock::time_point ultima_caida;

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
        : tablero(ALTO, vector<TipoPieza>(ANCHO, TipoPieza::VACIO))
        , colores_tablero(ALTO, vector<ColorTetris>(ANCHO, ColorTetris::GRIS))
        , pieza_actual(nullptr)
        , siguiente_pieza(nullptr)
        , generador(chrono::system_clock::now().time_since_epoch().count())
        , distribucion_piezas(0, 6)
        , puntos(0)
        , nivel(1)
        , lineas_completadas(0)
        , velocidad_caida(800)
        , juego_activo(true)
        , pausado(false)
        , game_over(false) {
        auto ahora = high_resolution_clock::now();
        ultimo_movimiento = ahora;
        ultima_caida      = ahora;
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
            siguiente_pieza = nullptr;
        } else {
            pieza_actual = new PiezaTetris(static_cast<TipoPieza>(distribucion_piezas(generador)), config);
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
        siguiente_pieza = new PiezaTetris(static_cast<TipoPieza>(distribucion_piezas(generador)), config);
    }

    bool esMovimientoValido(int nx, int ny, int nr) {
        if (!pieza_actual) return false;

        auto& forma = pieza_actual->rotaciones[nr];
        int   h     = static_cast<int>(forma.size());
        int   w     = static_cast<int>(forma[0].size());

        for (int py = 0; py < h; ++py) {
            for (int px = 0; px < w; ++px) {
                if (forma[py][px] == 1) {
                    int wx = nx + px;
                    int wy = ny + py;
                    if (wx < 0 || wx >= ANCHO || wy >= ALTO) return false;
                    if (wy >= 0 && tablero[wy][wx] != TipoPieza::VACIO) return false;
                }
            }
        }
        return true;
    }

    void fijarPieza() {
        if (!pieza_actual) return;
        auto& forma = pieza_actual->rotaciones[pieza_actual->rotacion_actual];
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
                if (tablero[y][x] == TipoPieza::VACIO) {
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
                tablero[0][x]        = TipoPieza::VACIO;
                colores_tablero[0][x] = ColorTetris::GRIS;
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
                    puntos += 10;
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
                tablero[y][x]        = TipoPieza::VACIO;
                colores_tablero[y][x] = ColorTetris::GRIS;
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
        pieza_actual   = nullptr;
        siguiente_pieza = nullptr;

        generarNuevaPieza();
        generarSiguientePieza();

        auto ahora = high_resolution_clock::now();
        ultimo_movimiento = ahora;
        ultima_caida      = ahora;
    }

    void actualizarFisica() {
        if (pausado || game_over || !pieza_actual) return;
        auto ahora = high_resolution_clock::now();
        auto dt    = duration_cast<milliseconds>(ahora - ultima_caida).count();
        if (dt >= velocidad_caida) {
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
                    auto& f = pieza_actual->rotaciones[pieza_actual->rotacion_actual];
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
                    if (tablero[y][x] != TipoPieza::VACIO) {
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
                auto& f = siguiente_pieza->rotaciones[0];
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

        // FPS
        static auto ultimo_fps = high_resolution_clock::now();
        static int  fps        = 0;
        fps++;
        auto ahora = high_resolution_clock::now();
        auto ms    = duration_cast<milliseconds>(ahora - ultimo_fps).count();
        if (ms >= 1000) {
            buf << "\033[92mFPS: " << fps << " | Cargado desde AST\033[0m\n";
            fps = 0;
            ultimo_fps = ahora;
        }

        cout << buf.str();
        cout.flush();
    }

    void ejecutar() {
        const int  TARGET_FPS = 60;
        const auto FRAME      = microseconds(1000000 / TARGET_FPS);
        auto       proximo    = high_resolution_clock::now() + FRAME;
        while (juego_activo) {
            auto inicio = high_resolution_clock::now();
            procesarEntrada();
            if (!pausado && !game_over) {
                actualizarFisica();
            }
            if (inicio >= proximo) {
                renderizar();
                proximo += FRAME;
            }
            auto restante = proximo - high_resolution_clock::now();
            if (restante > microseconds(0)) {
                auto sleep_ms = duration_cast<milliseconds>(restante).count();
                if (sleep_ms > 0) {
                    Sleep(static_cast<DWORD>(sleep_ms));
                }
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
    mt19937            generador;
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
         generador(random_device{}()) {
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
        uniform_int_distribution<int> dx(1, ancho_tablero - 2);
        uniform_int_distribution<int> dy(1, alto_tablero - 2);
        do {
            fruta_posicion = Posicion(dx(generador), dy(generador));
        } while (esPosicionOcupadaPorSnake(fruta_posicion));
    }

    bool esPosicionOcupadaPorSnake(const Posicion& p) {
        for (const auto& s : cuerpo_snake) {
            if (s == p) return true;
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
            if (p != string::npos) msg.replace(p, 8, to_string(puntos));
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
                            auto colores = config.arrays.count("colores_snake")
                                         ? config.arrays["colores_snake"]
                                         : vector<string>{"verde_claro", "verde_oscuro", "verde_medio"};
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
        const auto FRAME_DURATION = milliseconds(velocidad_ms);
        auto       proximo        = high_resolution_clock::now() + FRAME_DURATION;
        while (juego_activo) {
            auto inicio = high_resolution_clock::now();
            procesarEntrada();
            if (inicio >= proximo) {
                actualizarFisica();
                proximo += FRAME_DURATION;
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

int main(){
    cout << "============================================\n";
    cout << "           RUNTIME - SELECCION DE JUEGO\n";
    cout << "============================================\n";
    cout << "1) Tetris\n2) Snake\n3) Salir\n";
    cout << "Elige una opcion (1-3): ";
    int op = 0;
    cin >> op;
    cout << "\n";

    switch(op){
        case 1: {
            cout << "Compilando configuracion de Tetris y lanzando juego...\n";
            compilarJuegoSiPosible("tetris");
            TetrisEngine t;
            t.ejecutar();
            cout << "\nGracias por jugar Tetris!\n";
            break;
        }
        case 2: {
            cout << "Compilando configuracion de Snake y lanzando juego...\n";
            compilarJuegoSiPosible("snake");
            SnakeEngine s;
            s.ejecutar();
            cout << "\nGracias por jugar Snake!\n";
            break;
        }
        default:
            cout << "Saliendo...\n";
    }
    return 0;
}

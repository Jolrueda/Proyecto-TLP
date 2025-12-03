# ============================================
# MAKEFILE - MOTOR DE LADRILLOS
# ============================================

# Configuración del compilador
CXX = g++
CXXFLAGS = -std=gnu++98 -Wall -Wextra -O2

# Flags para SDL2 (modo gráfico)
SDL_CXXFLAGS = -DUSE_SDL
SDL_LDFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf

# Directorios
SRCDIR = src
BUILDDIR = build
BINDIR = bin
CONFIGDIR = config/games

# Archivos fuente
COMPILADOR_SRC = $(SRCDIR)/compilador.cpp
RUNTIME_SRC = $(SRCDIR)/runtime.cpp

# Ejecutables
COMPILADOR_EXE = $(BINDIR)/compilador.exe
RUNTIME_EXE = $(BINDIR)/runtime.exe
RUNTIME_SDL_EXE = $(BINDIR)/runtime_sdl.exe

# Archivos de configuración
TETRIS_CONFIG = $(CONFIGDIR)/Tetris.brik
SNAKE_CONFIG = $(CONFIGDIR)/Snake.brik
AST_FILE = $(BUILDDIR)/arbol.ast

# ============================================
# TARGETS PRINCIPALES
# ============================================

.PHONY: all clean info help demo run compilador tetris snake runtime runtime-sdl play play-sdl

# Target por defecto
all: $(COMPILADOR_EXE) $(RUNTIME_EXE)
	@echo.
	@echo ============================================
	@echo   COMPILACION COMPLETADA
	@echo ============================================
	@echo Ejecutables disponibles:
	@echo   - $(COMPILADOR_EXE)
	@echo   - $(RUNTIME_EXE) (modo consola)
	@echo.
	@echo Para compilar con soporte grafico (SDL2):
	@echo   make runtime-sdl
	@echo.
	@echo Usa 'make help' para ver todas las opciones

# Ayuda
help:
	@echo ============================================
	@echo   MOTOR DE LADRILLOS - SISTEMA DE BUILD
	@echo ============================================
	@echo.
	@echo Targets disponibles:
	@echo   make all         - Compilar todo (compilador + runtime consola)
	@echo   make compilador  - Solo compilar el compilador .brik
	@echo   make runtime     - Compilar runtime (modo consola)
	@echo   make runtime-sdl - Compilar runtime con SDL2 (modo grafico)
	@echo.
	@echo   make tetris      - Compilar Tetris.brik y ejecutar runtime
	@echo   make snake       - Compilar Snake.brik y ejecutar runtime  
	@echo   make play        - Ejecutar runtime (selector de juegos)
	@echo   make play-sdl    - Ejecutar runtime con SDL2
	@echo.
	@echo   make clean       - Limpiar archivos generados
	@echo   make info        - Informacion del proyecto
	@echo   make help        - Mostrar esta ayuda
	@echo.
	@echo NOTA: Para usar modo grafico necesitas tener instalado SDL2 y SDL2_ttf

info:
	@echo ============================================
	@echo   MOTOR DE LADRILLOS - INFORMACION
	@echo ============================================
	@echo Compilador: $(CXX) $(CXXFLAGS)
	@echo Directorios:
	@echo   - Fuentes: $(SRCDIR)/
	@echo   - Build: $(BUILDDIR)/
	@echo   - Binarios: $(BINDIR)/
	@echo   - Configs: $(CONFIGDIR)/
	@echo.
	@echo Archivos disponibles:
	@echo   - $(TETRIS_CONFIG)
	@echo   - $(SNAKE_CONFIG)
	@echo.
	@echo Modos de renderizado:
	@echo   - Consola: runtime.exe (por defecto)
	@echo   - Ventana grafica: runtime_sdl.exe (requiere SDL2)

# ============================================
# COMPILACION DE COMPONENTES
# ============================================

# Compilador .brik
compilador: $(COMPILADOR_EXE)
	@echo Compilador .brik listo: $(COMPILADOR_EXE)

$(COMPILADOR_EXE): $(COMPILADOR_SRC) | $(BINDIR)
	@echo [CC] Compilando compilador.cpp...
	$(CXX) $(CXXFLAGS) -o $@ $<

# Runtime (modo consola - sin SDL)
runtime: $(RUNTIME_EXE)
	@echo Runtime (consola) listo: $(RUNTIME_EXE)

$(RUNTIME_EXE): $(RUNTIME_SRC) | $(BINDIR)
	@echo [CC] Compilando runtime.cpp (modo consola)...
	$(CXX) $(CXXFLAGS) -o $@ $<

# Runtime con SDL2 (modo grafico)
runtime-sdl: $(RUNTIME_SDL_EXE)
	@echo Runtime (SDL2) listo: $(RUNTIME_SDL_EXE)

$(RUNTIME_SDL_EXE): $(RUNTIME_SRC) | $(BINDIR)
	@echo [CC] Compilando runtime.cpp (modo SDL2)...
	$(CXX) $(CXXFLAGS) $(SDL_CXXFLAGS) -o $@ $< $(SDL_LDFLAGS)

# ============================================
# COMPILACION Y EJECUCION DE JUEGOS
# ============================================

# Compilar Tetris y ejecutar
tetris: $(COMPILADOR_EXE) $(RUNTIME_EXE)
	@echo.
	@echo ============================================
	@echo   COMPILANDO Y EJECUTANDO TETRIS
	@echo ============================================
	$(COMPILADOR_EXE) $(TETRIS_CONFIG)
	@echo.
	@echo Iniciando Tetris...
	@echo Presiona cualquier tecla en la ventana para empezar
	@echo.
	$(RUNTIME_EXE)

# Compilar Snake y ejecutar
snake: $(COMPILADOR_EXE) $(RUNTIME_EXE)
	@echo.
	@echo ============================================
	@echo   COMPILANDO Y EJECUTANDO SNAKE
	@echo ============================================
	$(COMPILADOR_EXE) $(SNAKE_CONFIG)
	@echo.
	@echo Iniciando Snake...
	@echo Presiona cualquier tecla en la ventana para empezar
	@echo.
	$(RUNTIME_EXE)

# Ejecutar runtime selector
play: $(RUNTIME_EXE)
	@echo Ejecutando runtime (selector de juegos - consola)...
	$(RUNTIME_EXE)

# Ejecutar runtime SDL
play-sdl: $(RUNTIME_SDL_EXE)
	@echo Ejecutando runtime (selector de juegos - SDL2)...
	$(RUNTIME_SDL_EXE)

# ============================================
# UTILIDADES
# ============================================

# Crear directorios necesarios
$(BUILDDIR):
	@if not exist $(BUILDDIR) mkdir $(BUILDDIR)

$(BINDIR):
	@if not exist $(BINDIR) mkdir $(BINDIR)

# Limpiar archivos generados
clean:
	@echo Limpiando archivos generados...
	@if exist $(BUILDDIR)\*.o del $(BUILDDIR)\*.o
	@if exist $(COMPILADOR_EXE) del $(COMPILADOR_EXE)
	@if exist $(RUNTIME_EXE) del $(RUNTIME_EXE)
	@if exist $(RUNTIME_SDL_EXE) del $(RUNTIME_SDL_EXE)
	@if exist $(AST_FILE) del $(AST_FILE)
	@echo Limpieza completada.

# ============================================
# DEPENDENCIAS
# ============================================

# Los fuentes no dependen de headers externos actualmente
# ============================================
# MAKEFILE - MOTOR DE LADRILLOS
# ============================================

# Configuración del compilador
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2

# Directorios
SRCDIR = src
BUILDDIR = build
BINDIR = bin
CONFIGDIR = config/games

# Archivos fuente
COMPILADOR_SRC = $(SRCDIR)/compilador.cpp
RUNTIME_SRC = $(SRCDIR)/runtime.cpp

# Ejecutables (solo runtime.exe con GDI)
COMPILADOR_EXE = $(BINDIR)/compilador.exe
RUNTIME_EXE = $(BINDIR)/runtime.exe

# Archivos de configuración
TETRIS_CONFIG = $(CONFIGDIR)/Tetris.brik
SNAKE_CONFIG = $(CONFIGDIR)/Snake.brik
AST_FILE = $(BUILDDIR)/arbol.ast

# ============================================
# TARGETS PRINCIPALES
# ============================================

.PHONY: all clean info help demo run compilador tetris snake runtime play

# Target por defecto
all: $(COMPILADOR_EXE) $(RUNTIME_EXE)
	@echo.
	@echo ============================================
	@echo   COMPILACION COMPLETADA
	@echo ============================================
	@echo Ejecutables disponibles:
	@echo   - $(COMPILADOR_EXE)
	@echo   - $(RUNTIME_EXE) (con GDI y consola)
	@echo.
	@echo Usa 'make help' para ver todas las opciones

# Ayuda
help:
	@echo ============================================
	@echo   MOTOR DE LADRILLOS - SISTEMA DE BUILD
	@echo ============================================
	@echo.
	@echo Targets disponibles:
	@echo   make all         - Compilar todo (compilador + runtime con GDI)
	@echo   make compilador  - Solo compilar el compilador .brik
	@echo   make runtime     - Compilar runtime.exe (con GDI incluido)
	@echo.
	@echo   make tetris      - Compilar Tetris.brik y ejecutar runtime
	@echo   make snake       - Compilar Snake.brik y ejecutar runtime  
	@echo   make play        - Ejecutar runtime (selector de juegos)
	@echo.
	@echo   make clean       - Limpiar archivos generados
	@echo   make info        - Informacion del proyecto
	@echo   make help        - Mostrar esta ayuda
	@echo.
	@echo NOTA: runtime.exe incluye soporte grafico GDI y modo consola

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
	@echo   - runtime.exe incluye: Consola y Ventana grafica (GDI)

# ============================================
# COMPILACION DE COMPONENTES
# ============================================

# Compilador .brik
compilador: $(COMPILADOR_EXE)
	@echo Compilador .brik listo: $(COMPILADOR_EXE)

$(COMPILADOR_EXE): $(COMPILADOR_SRC) | $(BINDIR)
	@echo [CC] Compilando compilador.cpp...
	$(CXX) $(CXXFLAGS) -o $@ $<

# Runtime (con GDI incluido)
runtime: $(RUNTIME_EXE)
	@echo Runtime listo: $(RUNTIME_EXE)

$(RUNTIME_EXE): $(RUNTIME_SRC) | $(BINDIR)
	@echo [CC] Compilando runtime.cpp (con GDI - Win32)...
	$(CXX) $(CXXFLAGS) -DUSE_GDI -o $@ $< -lgdi32 -luser32

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
	@if exist $(AST_FILE) del $(AST_FILE)
	@echo Limpieza completada.

# ============================================
# DEPENDENCIAS
# ============================================

# Los fuentes no dependen de headers externos actualmente
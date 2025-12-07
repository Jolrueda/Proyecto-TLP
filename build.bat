@echo off
REM ============================================
REM BUILD SCRIPT - MOTOR DE LADRILLOS
REM ============================================

set CXX=g++
set CXXFLAGS=-std=c++11 -Wall -Wextra -O2

REM Directorios
set SRCDIR=src
set BUILDDIR=build
set BINDIR=bin
set CONFIGDIR=config\games

REM Archivos fuente
set COMPILADOR_SRC=%SRCDIR%\compilador.cpp
set RUNTIME_SRC=%SRCDIR%\runtime.cpp

REM Ejecutables (solo runtime.exe con GDI)
set COMPILADOR_EXE=%BINDIR%\compilador.exe
set RUNTIME_EXE=%BINDIR%\runtime.exe

REM Archivos de configuración
set TETRIS_CONFIG=%CONFIGDIR%\Tetris.brik
set SNAKE_CONFIG=%CONFIGDIR%\Snake.brik
set AST_FILE=%BUILDDIR%\arbol.ast

REM Crear directorios si no existen
if not exist %BUILDDIR% mkdir %BUILDDIR%
if not exist %BINDIR% mkdir %BINDIR%

if "%1"=="help" goto help
if "%1"=="clean" goto clean
if "%1"=="compilador" goto compilador
if "%1"=="runtime" goto runtime
if "%1"=="tetris" goto tetris
if "%1"=="snake" goto snake
if "%1"=="play" goto play
if "%1"=="info" goto info
if "%1"=="all" goto all
if "%1"=="" goto all

goto help

:help
echo ============================================
echo   MOTOR DE LADRILLOS - SISTEMA DE BUILD
echo ============================================
echo.
echo Comandos disponibles:
echo   build.bat all         - Compilar todo (compilador + runtime con GDI)
echo   build.bat compilador  - Solo compilar el compilador .brik
echo   build.bat runtime     - Compilar runtime.exe (con GDI incluido)
echo.
echo   build.bat tetris      - Compilar Tetris.brik y ejecutar runtime
echo   build.bat snake       - Compilar Snake.brik y ejecutar runtime  
echo   build.bat play        - Ejecutar runtime (selector de juegos)
echo.
echo   build.bat clean       - Limpiar archivos generados
echo   build.bat info        - Informacion del proyecto
echo   build.bat help        - Mostrar esta ayuda
echo.
echo NOTA: runtime.exe incluye soporte grafico GDI y modo consola
goto end

:info
echo ============================================
echo   MOTOR DE LADRILLOS - INFORMACION
echo ============================================
echo Compilador: %CXX% %CXXFLAGS%
echo Directorios:
echo   - Fuentes: %SRCDIR%\
echo   - Build: %BUILDDIR%\
echo   - Binarios: %BINDIR%\
echo   - Configs: %CONFIGDIR%\
echo.
echo Archivos disponibles:
echo   - %TETRIS_CONFIG%
echo   - %SNAKE_CONFIG%
echo.
echo Modos de renderizado:
echo   - runtime.exe incluye: Consola y Ventana grafica (GDI)
goto end

:all
echo ============================================
echo   COMPILANDO TODO EL PROYECTO
echo ============================================
call :compilador
call :runtime
echo.
echo ============================================
echo   COMPILACION COMPLETADA
echo ============================================
echo Ejecutables disponibles:
echo   - %COMPILADOR_EXE%
echo   - %RUNTIME_EXE% (con GDI y consola)
echo.
echo Usa 'build.bat help' para ver todas las opciones
goto end

:compilador
echo [CC] Compilando compilador.cpp...
%CXX% %CXXFLAGS% -o %COMPILADOR_EXE% %COMPILADOR_SRC%
if errorlevel 1 (
    echo ERROR: Fallo al compilar compilador.cpp
    goto end
)
echo Compilador .brik listo: %COMPILADOR_EXE%
goto :eof

:runtime
echo [CC] Compilando runtime.cpp (con GDI - Win32)...
%CXX% %CXXFLAGS% -DUSE_GDI -o %RUNTIME_EXE% %RUNTIME_SRC% -lgdi32 -luser32
if errorlevel 1 (
    echo ERROR: Fallo al compilar runtime.cpp
    goto end
)
echo Runtime listo: %RUNTIME_EXE%
goto :eof

:tetris
call :compilador
call :runtime
echo.
echo ============================================
echo   COMPILANDO Y EJECUTANDO TETRIS
echo ============================================
%COMPILADOR_EXE% %TETRIS_CONFIG%
if errorlevel 1 (
    echo ERROR: Fallo al compilar Tetris.brik
    goto end
)
echo.
echo Iniciando Tetris...
echo.
%RUNTIME_EXE%
goto end

:snake
call :compilador
call :runtime
echo.
echo ============================================
echo   COMPILANDO Y EJECUTANDO SNAKE
echo ============================================
%COMPILADOR_EXE% %SNAKE_CONFIG%
if errorlevel 1 (
    echo ERROR: Fallo al compilar Snake.brik
    goto end
)
echo.
echo Iniciando Snake...
echo.
%RUNTIME_EXE%
goto end

:play
if not exist %RUNTIME_EXE% call :runtime
echo Ejecutando runtime (selector de juegos - consola)...
%RUNTIME_EXE%
goto end

:clean
echo Limpiando archivos generados...
if exist %BUILDDIR%\*.o del %BUILDDIR%\*.o
if exist %COMPILADOR_EXE% del %COMPILADOR_EXE%
if exist %RUNTIME_EXE% del %RUNTIME_EXE%
if exist %AST_FILE% del %AST_FILE%
echo Limpieza completada.
goto end

:end
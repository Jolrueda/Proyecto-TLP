# Motor de Ladrillos 🎮

  

Un motor de juegos desarrollado en C++ que incluye su propio lenguaje de configuración `.brik` para crear y configurar juegos de tipo "ladrillos" como Tetris y Snake.

  

## 📋 Descripción

  

El Motor de Ladrillos es un proyecto académico de **Teoría de Lenguajes de Programación** que implementa:

  

-  **Compilador personalizado** para archivos de configuración `.brik`

-  **Analizador léxico y sintáctico** completo

-  **Motor de renderizado** optimizado para juegos de ladrillos

-  **Runtime unificado** que permite seleccionar entre diferentes juegos

-  **Implementaciones de Tetris y Snake** como casos de uso

  

## 🚀 Características

  

### Lenguaje .brik

- Sintaxis declarativa simple para configurar juegos

- Soporte para tipos primitivos (strings, números, booleanos)

- Configuración de pantalla, tablero, colores y mecánicas de juego

- Sistema de comentarios con `//`

  

### Motor de Juegos

- Renderizado sin parpadeo optimizado para consola

- Manejo de colores ANSI y Windows Console

- Sistema de puntuación y niveles

- Controles personalizables

- Física de juego configurable

  

### Juegos Incluidos

-  **Tetris Clásico**: Con sistema de rotación, líneas completadas y niveles progresivos

-  **Snake**: Con crecimiento, colisiones y sistema de puntuación

  

## 🛠️ Compilación

  

### Requisitos

-  **Compilador**: g++ con soporte para C++14

-  **Sistema Operativo**: Windows (optimizado para Windows Console API)

-  **Make** (opcional): Para usar el sistema de build con Makefile

-  **Alternativa Windows**: Script `build.bat` incluido

  

### Compilar todo el proyecto

  

**Con Make (Linux/WSL/MSYS2):**

```bash

make  all

```

  

**Con build.bat (Windows nativo):**

```batch

build.bat all

```

  

### Compilar componentes individuales

  

**Con Make:**

```bash

make  compilador  # Solo el compilador .brik

make  runtime  # Solo el runtime selector

```

  

**Con build.bat:**

```batch

build.bat compilador # Solo el compilador .brik

build.bat runtime # Solo el runtime selector

```

  

## 🎮 Uso

  

### Opción 1: Ejecutar juegos individuales

  

#### Tetris

**Con Make:**

```bash

make  tetris

```

**Con build.bat:**

```batch

build.bat tetris

```

  

#### Snake

**Con Make:**

```bash

make  snake

```

**Con build.bat:**

```batch

build.bat snake

```

  

### Opción 2: Usar el selector de juegos

**Con Make:**

```bash

make  play

```

**Con build.bat:**

```batch

build.bat play

```

  

### Opción 3: Ejecutar manualmente

  

1.  **Compilar un archivo .brik:**

```batch

bin\compilador.exe config\games\Tetris.brik

```

  

2.  **Ejecutar el juego:**

```batch

bin\runtime.exe

```

  

## 📁 Estructura del Proyecto

  

```

Proyecto-TLP/

├── src/ # Código fuente

│ ├── compilador.cpp # Compilador del lenguaje .brik

│ └── runtime.cpp # Runtime unificado con ambos juegos

├── config/games/ # Archivos de configuración

│ ├── Tetris.brik # Configuración de Tetris

│ └── Snake.brik # Configuración de Snake

├── bin/ # Ejecutables generados

├── build/ # Archivos objeto y AST generado

├── Makefile # Sistema de build (Linux/WSL/MSYS2)

├── build.bat # Script de build (Windows nativo)

└── README.md # Este archivo

```

## 🎯 Comandos de Build

  

### Make (Linux/WSL/MSYS2)

| Comando | Descripción |

|---------|-------------|

| `make all` | Compila todo el proyecto |

| `make compilador` | Solo compilar el compilador .brik |

| `make runtime` | Solo compilar el runtime |

| `make tetris` | Compila y ejecuta Tetris |

| `make snake` | Compila y ejecuta Snake |

| `make play` | Ejecuta el selector de juegos |

| `make clean` | Limpia archivos generados |

| `make help` | Muestra ayuda detallada |

| `make info` | Información del proyecto |

  

### build.bat (Windows nativo)

| Comando | Descripción |

|---------|-------------|

| `build.bat all` | Compila todo el proyecto |

| `build.bat compilador` | Solo compilar el compilador .brik |

| `build.bat runtime` | Solo compilar el runtime |

| `build.bat tetris` | Compila y ejecuta Tetris |

| `build.bat snake` | Compila y ejecuta Snake |

| `build.bat play` | Ejecuta el selector de juegos |

| `build.bat clean` | Limpia archivos generados |

| `build.bat help` | Muestra ayuda detallada |

| `build.bat info` | Información del proyecto |

  

## 🕹️ Controles

  

### Tetris

-  **A/D**: Mover pieza horizontalmente

-  **S**: Acelerar caída

-  **SPACE**: Rotar pieza

-  **ESC**: Salir del juego

  

### Snake

-  **A/D/W/S**: Cambiar dirección

-  **ESC**: Salir del juego

  

## 🔧 Configuración

  

Los archivos `.brik` permiten personalizar completamente la experiencia de juego:

  

### Tetris (`config/games/Tetris.brik`)

- Dimensiones del tablero

- Velocidad y aceleración

- Colores de las piezas

- Sistema de puntuación

- Configuración de niveles

  

### Snake (`config/games/Snake.brik`)

- Tamaño del tablero

- Velocidad inicial de la serpiente

- Colores de serpiente y comida

- Mecánicas de crecimiento

- Sistema de puntuación

  

## 🏗️ Arquitectura

  

### Compilador

1.  **Analizador Léxico**: Tokeniza el código fuente .brik

2.  **Analizador Sintáctico**: Genera un AST (Árbol de Sintaxis Abstracta)

3.  **Generador**: Produce archivos de configuración para el motor

  

### Motor de Juegos

1.  **Parser AST**: Lee la configuración compilada

2.  **Inicializador**: Configura ventana, colores y recursos

3.  **Game Loop**: Maneja entrada, lógica y renderizado

4.  **Renderer**: Dibuja en consola con optimizaciones anti-parpadeo

  

  

## 📚 Conceptos Académicos Implementados

  

-  **Análisis Léxico**: Tokenización de código fuente

-  **Análisis Sintáctico**: Construcción de AST

-  **Lenguajes Específicos de Dominio (DSL)**: Lenguaje .brik

-  **Interpretación**: Ejecución del código compilado

-  **Optimización**: Renderizado eficiente en consola

  



  

## 📄 Licencia

  

Proyecto académico desarrollado para el curso de Teoría de Lenguajes de Programación.

  


  

---

  

**¿Problemas?** Usa `make help` para ver todas las opciones disponibles o revisa los logs de compilación para diagnosticar errores.
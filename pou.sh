#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 1. Create assets dir if missing
mkdir -p assets

# 2. Locate vcpkg
if [ -z "$VCPKG_ROOT" ]; then
    if [ -d "$HOME/vcpkg" ]; then
        export VCPKG_ROOT="$HOME/vcpkg"
    elif [ -d "/opt/vcpkg" ]; then
        export VCPKG_ROOT="/opt/vcpkg"
    fi
fi

if [ -z "$VCPKG_ROOT" ] || [ ! -f "$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" ]; then
    echo "[ERROR] No se encontró VCPKG_ROOT."
    echo "Por favor clona vcpkg en ~/vcpkg:"
    echo "  git clone https://github.com/microsoft/vcpkg.git ~/vcpkg"
    echo "  ~/vcpkg/bootstrap-vcpkg.sh"
    echo "  export VCPKG_ROOT=~/vcpkg"
    exit 1
fi

echo "[POU] Usando vcpkg en: $VCPKG_ROOT"

# 3. Configure CMake if not configured
if [ ! -f "build/build.ninja" ] && [ ! -f "build/Makefile" ]; then
    echo "[POU] Configurando el proyecto por primera vez y descargando dependencias..."
    cmake --preset linux
fi

# 4. Build
echo "[POU] Compilando MotorVJ..."
cmake --build build -j"$(nproc)"

# 5. Symlink compile_commands.json for Neovim / clangd
if [ -f "build/compile_commands.json" ] && [ ! -e "compile_commands.json" ]; then
    ln -sf build/compile_commands.json compile_commands.json
    echo "[POU] Enlace simbólico compile_commands.json creado para Neovim/clangd."
fi

# 6. Run if 'run' or '--run' argument is provided
if [ "$1" == "run" ] || [ "$1" == "--run" ]; then
    echo "[POU] Ejecutando MotorVJ..."
    ./build/MotorVJ
fi

echo "[POU] Compilación completada con éxito."

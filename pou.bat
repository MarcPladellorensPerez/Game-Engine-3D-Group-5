@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

REM Crear carpeta assets si no existe
if not exist "assets" mkdir "assets"

REM Detectar la instalacion de Visual Studio mediante vswhere
set "VS_PATH="
set "VSWHERE_PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "!VSWHERE_PATH!" (
    for /f "usebackq tokens=*" %%i in (`"!VSWHERE_PATH!" -latest -prerelease -property installationPath`) do (
        set "VS_PATH=%%i"
    )
)

REM Buscar el ejecutable de CMake (preferir el que viene integrado en Visual Studio)
set "CMAKE_EXE=cmake"
if defined VS_PATH (
    if exist "!VS_PATH!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "CMAKE_EXE=!VS_PATH!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    )
)

REM Buscar el archivo de toolchain de vcpkg
set "VCPKG_TOOLCHAIN="
if defined VCPKG_ROOT if exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    set "VCPKG_TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
)
if not defined VCPKG_TOOLCHAIN if defined VS_PATH (
    if exist "!VS_PATH!\VC\vcpkg\scripts\buildsystems\vcpkg.cmake" (
        set "VCPKG_TOOLCHAIN=!VS_PATH!\VC\vcpkg\scripts\buildsystems\vcpkg.cmake"
    )
)
if not defined VCPKG_TOOLCHAIN if exist "C:\vcpkg\scripts\buildsystems\vcpkg.cmake" (
    set "VCPKG_TOOLCHAIN=C:\vcpkg\scripts\buildsystems\vcpkg.cmake"
)

REM Si no existe la solucion de Visual Studio o falta configurar, generarla con CMake
if not exist "build\MotorVJ.sln" (
    echo [POU] Configurando el proyecto por primera vez y descargando dependencias...
    if defined VCPKG_TOOLCHAIN (
        "!CMAKE_EXE!" -B build -S . -DCMAKE_TOOLCHAIN_FILE="!VCPKG_TOOLCHAIN!"
    ) else (
        "!CMAKE_EXE!" -B build -S .
    )
    if errorlevel 1 (
        echo [ERROR] Hubo un problema al configurar el proyecto con CMake.
        pause
        exit /b 1
    )
)

REM Abrir la solucion en Visual Studio con MotorVJ listo como proyecto de inicio
echo [POU] Abriendo el proyecto en Visual Studio...
if defined VS_PATH if exist "!VS_PATH!\Common7\IDE\devenv.exe" (
    start "" "!VS_PATH!\Common7\IDE\devenv.exe" "%~dp0build\MotorVJ.sln"
) else (
    start "" "%~dp0build\MotorVJ.sln"
)

exit /b 0

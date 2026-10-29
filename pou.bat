@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

REM 1. Crear directorio de assets si no existe
if not exist "assets" mkdir "assets"

REM 2. Detectar instalación de Visual Studio mediante vswhere
set "VS_PATH="
set "VSWHERE_PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "!VSWHERE_PATH!" (
    for /f "usebackq tokens=*" %%i in (`"!VSWHERE_PATH!" -latest -prerelease -property installationPath`) do (
        set "VS_PATH=%%i"
    )
)

REM 3. Localizar CMake (preferir el CMake integrado en Visual Studio si existe)
set "CMAKE_EXE=cmake"
if defined VS_PATH (
    if exist "!VS_PATH!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "CMAKE_EXE=!VS_PATH!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    )
)

REM 4. Localizar vcpkg toolchain
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
if not defined VCPKG_TOOLCHAIN if exist "%USERPROFILE%\vcpkg\scripts\buildsystems\vcpkg.cmake" (
    set "VCPKG_TOOLCHAIN=%USERPROFILE%\vcpkg\scripts\buildsystems\vcpkg.cmake"
)
if not defined VCPKG_TOOLCHAIN if exist "%~dp0vcpkg\scripts\buildsystems\vcpkg.cmake" (
    set "VCPKG_TOOLCHAIN=%~dp0vcpkg\scripts\buildsystems\vcpkg.cmake"
)

REM Si no se encuentra vcpkg en ningún directorio habitual, descargarlo e inicializarlo automáticamente
if not defined VCPKG_TOOLCHAIN (
    where git >nul 2>&1
    if not errorlevel 1 (
        echo [POU] No se detecto vcpkg en este equipo.
        echo [POU] Descargando vcpkg en %USERPROFILE%\vcpkg...
        git clone https://github.com/microsoft/vcpkg.git "%USERPROFILE%\vcpkg"
        if exist "%USERPROFILE%\vcpkg\bootstrap-vcpkg.bat" (
            echo [POU] Compilando e inicializando vcpkg...
            call "%USERPROFILE%\vcpkg\bootstrap-vcpkg.bat" -disableMetrics
            set "VCPKG_TOOLCHAIN=%USERPROFILE%\vcpkg\scripts\buildsystems\vcpkg.cmake"
        )
    ) else (
        echo [AVISO] No se encontro vcpkg ni git en el PATH del sistema.
        echo Si CMake falla buscando librerias, instala vcpkg o define VCPKG_ROOT.
    )
)

REM 5. Generar la solución de Visual Studio con CMake si no existe
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

REM 6. Asegurar que Visual Studio seleccione MotorVJ como proyecto de inicio y fije el directorio de trabajo
if exist "build\MotorVJ.slnx" del /q "build\MotorVJ.slnx"

if not exist "build\MotorVJ.vcxproj.user" (
    (
        echo ^<?xml version="1.0" encoding="utf-8"?^>
        echo ^<Project ToolsVersion="Current" xmlns="http://schemas.microsoft.com/developer/msbuild/2003"^>
        echo   ^<PropertyGroup^>
        echo     ^<LocalDebuggerWorkingDirectory^>$(SolutionDir)..^</LocalDebuggerWorkingDirectory^>
        echo     ^<DebuggerFlavor^>WindowsLocalDebugger^</DebuggerFlavor^>
        echo   ^</PropertyGroup^>
        echo ^</Project^>
    ) > "build\MotorVJ.vcxproj.user"
)

REM 7. Abrir la solución en Visual Studio
echo [POU] Abriendo el proyecto en Visual Studio...
if defined VS_PATH if exist "!VS_PATH!\Common7\IDE\devenv.exe" (
    start "" "!VS_PATH!\Common7\IDE\devenv.exe" "%~dp0build\MotorVJ.sln"
) else (
    start "" "%~dp0build\MotorVJ.sln"
)

exit /b 0

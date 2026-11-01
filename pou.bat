@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

REM 1. Create assets dir if missing
if not exist "assets" mkdir "assets"

REM 2. Detect Visual Studio via vswhere
set "VS_PATH="
set "VSWHERE_PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "!VSWHERE_PATH!" (
    for /f "usebackq tokens=*" %%i in (`"!VSWHERE_PATH!" -latest -prerelease -property installationPath`) do (
        set "VS_PATH=%%i"
    )
)

REM Detect Visual Studio version for CMake generator
set "VS_GENERATOR=Visual Studio 17 2022"
if defined VS_PATH (
    if exist "!VS_PATH!\Common7\IDE\devenv.exe" (
        for /f "usebackq tokens=*" %%v in (`"!VSWHERE_PATH!" -latest -prerelease -property installationVersion`) do (
            set "VS_VERSION=%%v"
        )
        if "!VS_VERSION:~0,2!"=="18" set "VS_GENERATOR=Visual Studio 18 2026"
        if "!VS_VERSION:~0,2!"=="17" set "VS_GENERATOR=Visual Studio 17 2022"
        if "!VS_VERSION:~0,2!"=="16" set "VS_GENERATOR=Visual Studio 16 2019"
    )
)

REM 3. Locate CMake (prefer VS bundled CMake)
set "CMAKE_EXE=cmake"
if defined VS_PATH (
    if exist "!VS_PATH!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "CMAKE_EXE=!VS_PATH!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    )
)

REM 4. Locate vcpkg toolchain
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

REM Auto-bootstrap vcpkg if missing
if not defined VCPKG_TOOLCHAIN (
    where git >nul 2>&1
    if not errorlevel 1 (
        echo [POU] No se detecto vcpkg en este equipo.
        if exist "%USERPROFILE%\vcpkg" (
            REM vcpkg folder exists but was not compiled - bootstrap it
            echo [POU] Se encontro la carpeta vcpkg pero no estaba compilada. Compilando...
        ) else (
            echo [POU] Descargando vcpkg en %USERPROFILE%\vcpkg...
            git clone https://github.com/microsoft/vcpkg.git "%USERPROFILE%\vcpkg"
        )
        if exist "%USERPROFILE%\vcpkg\bootstrap-vcpkg.bat" (
            echo [POU] Compilando e inicializando vcpkg...
            call "%USERPROFILE%\vcpkg\bootstrap-vcpkg.bat" -disableMetrics
            set "VCPKG_TOOLCHAIN=%USERPROFILE%\vcpkg\scripts\buildsystems\vcpkg.cmake"
        ) else (
            echo [ERROR] No se pudo inicializar vcpkg.
            pause
            exit /b 1
        )
    ) else (
        echo [AVISO] No se encontro vcpkg ni git en el PATH del sistema.
        echo Instala Git desde https://git-scm.com o define la variable VCPKG_ROOT.
        pause
        exit /b 1
    )
)

REM 5. Generate VS solution with CMake
REM   Force regeneration if build/CMakeCache.txt is missing (catches failed/partial builds)
if not exist "build\CMakeCache.txt" (
    echo [POU] Configurando el proyecto con CMake (generador: !VS_GENERATOR!)...
    if defined VCPKG_TOOLCHAIN (
        "!CMAKE_EXE!" -B build -S . -G "!VS_GENERATOR!" -A x64 -DCMAKE_TOOLCHAIN_FILE="!VCPKG_TOOLCHAIN!"
    ) else (
        "!CMAKE_EXE!" -B build -S . -G "!VS_GENERATOR!" -A x64
    )
    if errorlevel 1 (
        echo [ERROR] Hubo un problema al configurar el proyecto con CMake.
        echo Revisa que tengas instalado Visual Studio con el componente "Desarrollo de escritorio con C++".
        pause
        exit /b 1
    )
)

REM 6. Set correct working directory for the debugger
if not exist "build\MotorVJ.vcxproj.user" (
    (
        echo ^<?xml version="1.0" encoding="utf-8"?^>
        echo ^<Project ToolsVersion="Current" xmlns="http://schemas.microsoft.com/developer/msbuild/2003"^>
        echo   ^<PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'"^>
        echo     ^<LocalDebuggerWorkingDirectory^>%~dp0^</LocalDebuggerWorkingDirectory^>
        echo     ^<DebuggerFlavor^>WindowsLocalDebugger^</DebuggerFlavor^>
        echo   ^</PropertyGroup^>
        echo   ^<PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'"^>
        echo     ^<LocalDebuggerWorkingDirectory^>%~dp0^</LocalDebuggerWorkingDirectory^>
        echo     ^<DebuggerFlavor^>WindowsLocalDebugger^</DebuggerFlavor^>
        echo   ^</PropertyGroup^>
        echo ^</Project^>
    ) > "build\MotorVJ.vcxproj.user"
)

REM 7. Open solution in Visual Studio
echo [POU] Abriendo el proyecto en Visual Studio...
if defined VS_PATH if exist "!VS_PATH!\Common7\IDE\devenv.exe" (
    start "" "!VS_PATH!\Common7\IDE\devenv.exe" "%~dp0build\MotorVJ.sln"
) else (
    if exist "%~dp0build\MotorVJ.sln" (
        start "" "%~dp0build\MotorVJ.sln"
    ) else (
        echo [ERROR] No se encontro MotorVJ.sln en la carpeta build\.
        echo Comprueba que CMake ha terminado correctamente.
        pause
        exit /b 1
    )
)

exit /b 0

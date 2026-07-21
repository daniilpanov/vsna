@echo off
setlocal enabledelayedexpansion

rem --- настройки по умолчанию ---
set "SCRIPT_DIR=%~dp0"
pushd "%SCRIPT_DIR%"

set "BUILD_DIR=%BUILD_DIR%"
if "%BUILD_DIR%"=="" set "BUILD_DIR=out"

set "VCPKG_DIR=%SCRIPT_DIR%vcpkg"
set "TRIPLET=x64-windows"
set "TYPE="
set "MODE="

:usage
echo Usage: %~nx0 ^<command^>
echo Commands:
echo --server Configure and build as a server
echo --client Configure and build as a client
echo --no-vcpkg Configure and build without vcpkg (via lib-devel)
echo --clean Remove build directory
popd
exit /b 0

:cmd_clean
if exist "%BUILD_DIR%" (
echo -- Removing %BUILD_DIR%
rd /s /q "%BUILD_DIR%" 2>nul
if errorlevel 1 echo Warning: failed to remove %BUILD_DIR%
) else (
echo -- %BUILD_DIR% not found
)
echo -- Clean done!
popd
exit /b 0

:cmd_build_no_vcpkg
if not exist "%BUILD_DIR%" md "%BUILD_DIR%"
echo -- Looking in local packages
cmake -S . -B "%BUILD_DIR%" ^
-DVCPKG_TARGET_TRIPLET=%TRIPLET% ^
-DSERVER=%SERVER_FLAG% ^
-DCLIENT=%CLIENT_FLAG%
if errorlevel 1 (
echo cmake configuration failed
popd
exit /b 1
)
cmake --build "%BUILD_DIR%"
if errorlevel 1 (
echo build failed
popd
exit /b 1
)
echo -- Build done!
popd
exit /b 0

:cmd_build_via_vcpkg
if not exist "%VCPKG_DIR%" (
echo -- Error: vcpkg not found. Run init_boost.bat or place vcpkg in %VCPKG_DIR%
popd
exit /b 1
)
if not exist "%BUILD_DIR%" md "%BUILD_DIR%"
echo -- Using vcpkg at %VCPKG_DIR%
cmake -S . -B "%BUILD_DIR%" ^
-DCMAKE_TOOLCHAIN_FILE="%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake" ^
-DVCPKG_TARGET_TRIPLET=%TRIPLET% ^
-DSERVER=%SERVER_FLAG% ^
-DCLIENT=%CLIENT_FLAG%
if errorlevel 1 (
echo cmake configuration failed
popd
exit /b 1
)
cmake --build "%BUILD_DIR%"
if errorlevel 1 (
echo build failed
popd
exit /b 1
)
echo -- Build done!
popd
exit /b 0

rem --- Разбор аргументов ---
:parse_loop
if "%~1"=="" goto parse_done

if /I "%~1"=="--clean" (
call :cmd_clean
goto :eof
) else if /I "%~1"=="-h" (
call :usage
goto :eof
) else if /I "%~1"=="--help" (
call :usage
goto :eof
) else if /I "%~1"=="--server" (
set "MODE=server"
) else if /I "%~1"=="--client" (
set "MODE=client"
) else if /I "%~1"=="--no-vcpkg" (
set "TYPE=--no-vcpkg"
) else (
echo Unknown arg: %~1
popd
exit /b 1
)
shift
goto parse_loop

:parse_done

if "%MODE%"=="" (
echo Usage: %~nx0 [--server|--client] [--no-vcpkg]
popd
exit /b 1
)

rem подготовка флагов для cmake (ON/OFF)
if /I "%MODE%"=="server" (
set "SERVER_FLAG=ON"
set "CLIENT_FLAG=OFF"
) else (
set "SERVER_FLAG=OFF"
set "CLIENT_FLAG=ON"
)

rem выбор сборки
if /I "%TYPE%"=="--no-vcpkg" (
call :cmd_build_no_vcpkg
) else (
call :cmd_build_via_vcpkg
)

rem safety
popd
exit /b 0
// end build.bat
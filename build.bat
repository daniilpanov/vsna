@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
pushd "%SCRIPT_DIR%"

set "OUT_DIR=%OUT_DIR%"
if "%OUT_DIR%"=="" set "OUT_DIR=out"

set "VCPKG_DIR=%SCRIPT_DIR%\vcpkg"
set "TRIPLET=x64-windows"
set "USE_VCPKG=true"
set "MODE="

goto parse_loop

:usage
echo Usage: %~nx0 ^<command^>
echo Commands:
echo   --server    Configure and build as a server
echo   --client    Configure and build as a client
echo   --no-vcpkg  Configure and build without vcpkg (via lib-devel)
echo   --clean     Remove build directory
exit /b 0


:cmd_build
set "BUILD_DIR=%OUT_DIR%\%MODE%"
if not exist "%BUILD_DIR%" md "%BUILD_DIR%"

if "%USE_VCPKG%"=="true" (
    if not exist "%VCPKG_DIR%" (
        echo -- vcpkg not found, initializing submodule...
        git submodule update --init vcpkg
        if errorlevel 1 (
            echo -- Failed to initialize vcpkg submodule
            exit /b 1
        )
    )
) else (
    echo -- Looking in local packages
)

if /I "%MODE%"=="server" (
    set "SERVER_FLAG=ON"
    set "CLIENT_FLAG=OFF"
) else (
    set "SERVER_FLAG=OFF"
    set "CLIENT_FLAG=ON"
)

if "%USE_VCPKG%"=="true" (
    (
        cmake -S . -B "%BUILD_DIR%" ^
            -DCMAKE_TOOLCHAIN_FILE="%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake" ^
            -DVCPKG_TARGET_TRIPLET=%TRIPLET% ^
            -DAPP_NAME="vsna_%MODE%" ^
            -DSERVER=%SERVER_FLAG% ^
            -DCLIENT=%CLIENT_FLAG%
        cmake --build "%BUILD_DIR%" --config Debug
    )
) else (
    (
        cmake -S . -B "%BUILD_DIR%" ^
            -DVCPKG_TARGET_TRIPLET=%TRIPLET% ^
            -DAPP_NAME="vsna_%MODE%" ^
            -DSERVER=%SERVER_FLAG% ^
            -DCLIENT=%CLIENT_FLAG%
        cmake --build "%BUILD_DIR%" --config Debug
    )
)

echo -- Build done!
exit /b 0


:cmd_clean
if exist "%OUT_DIR%" (
    rd /s /q "%OUT_DIR%"
    echo -- Removed %OUT_DIR%
)
echo -- Clean done!
exit /b 0


:parse_loop
if "%~1"=="" goto parse_done

if /I "%~1"=="--clean" (
    call :cmd_clean
    popd
    exit /b 0
) else if /I "%~1"=="-h" (
    call :usage
    popd
    exit /b 0
) else if /I "%~1"=="--help" (
    call :usage
    popd
    exit /b 0
) else if /I "%~1"=="--server" (
    set "MODE=server"
) else if /I "%~1"=="--client" (
    set "MODE=client"
) else if /I "%~1"=="--no-vcpkg" (
    set "USE_VCPKG=false"
) else (
    echo Unknown arg: %~1
    popd
    exit /b 1
)

shift
goto parse_loop


:parse_done
if "%MODE%"=="" (
    set "MODE=server"
    call :cmd_build

    set "MODE=client"
    call :cmd_build

    popd
    exit /b 0
) else (
    call :cmd_build
    popd
    exit /b 0
)


:cmd_end
popd
exit /b 0
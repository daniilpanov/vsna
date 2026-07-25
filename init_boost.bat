@echo off
setlocal enabledelayedexpansion

set "ROOT=%~dp0"
set "VCPKG_DIR=%ROOT%\vcpkg"
set "VCPKG_REPO=https://github.com/microsoft/vcpkg.git"
set "TRIPLET=x64-windows"

if "%~1" == "--init"  ( goto :cmd_init )
if "%~1" == "--clean" ( goto :cmd_clean )
if "%~1" == "--help"  ( goto :usage )

if "%~1" == "" (
    goto :cmd_init
) else (
    echo Unknown command: %~1
    goto :usage
)

:usage
echo Usage:
echo   --init   Init vcpkg and install boost
echo   --clean  Delete vcpkg folder
echo   --help   Look commands

:cmd_build
if not exist "%VCPKG_DIR%" (
    git clone %VCPKG_REPO% "%VCPKG_DIR%"
    if errorlevel 1 (
        echo Failed to clone vcpkg
        exit /b 1
    )
)

pushd "%VCPKG_DIR%"

call bootstrap-vcpkg.bat
if errorlevel 1 (
    echo Failed to bootstrap vcpkg
    popd
    exit /b 1
)

vcpkg install boost-filesystem boost-system boost-asio boost-beast --triplet %TRIPLET%
if errorlevel 1 (
    echo Failed to install boost
    popd
    exit /b 1
)

vcpkg integrate install
popd
goto :cmd_ending

:cmd_clean
if exist %VCPKG_DIR% (
    rmdir /s /q %VCPKG_DIR%
    echo -- Deleted %VCPKG_DIR%
) else (
    echo -- %VCPKG_DIR% is not exists
)
goto :cmd_ending

:cmd_ending
echo -- Done.
endlocal
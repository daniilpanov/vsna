@echo off
setlocal enabledelayedexpansion

set "ROOT=%~dp0"
set "VCPKG_DIR=%ROOT%\vcpkg"
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
echo   --init   Bootstrap vcpkg and install Boost
echo   --clean  Delete vcpkg folder
echo   --help   Show commands
goto :cmd_ending

:cmd_init
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
echo -- Installed Boost

goto :cmd_ending

:cmd_clean
if exist %VCPKG_DIR% (
    rmdir /s /q %VCPKG_DIR%
    echo -- Deleted %VCPKG_DIR%
) else (
    echo -- %VCPKG_DIR% does not exist
)
goto :cmd_ending

:cmd_ending
echo -- Done.
endlocal

@echo off
rem Builds the engine on Windows without needing a bash at all.
rem
rem Scripts/build_windows_mingw.sh does the same thing, but `bash` on a
rem Windows box is as likely to be WSL as Git Bash, and WSL without a
rem distribution installed just refuses. This one is cmd, so it always runs.

setlocal enabledelayedexpansion
cd /d "%~dp0"

title Conquer the Spire - building

rem ------------------------------------------------------------------ cmake
set "CMAKE="
where cmake >nul 2>&1 && set "CMAKE=cmake"
if not defined CMAKE if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "CMAKE=%ProgramFiles%\CMake\bin\cmake.exe"
if not defined CMAKE if exist "%ProgramFiles(x86)%\CMake\bin\cmake.exe" set "CMAKE=%ProgramFiles(x86)%\CMake\bin\cmake.exe"

if defined CMAKE goto haveCMake
echo CMake was not found. Install it with:
echo     winget install --id Kitware.CMake
echo.
pause
exit /b 1

:haveCMake

rem ------------------------------------------------------- the compiler, and
rem the make it ships with. Neither w64devkit nor WinLibs is on PATH by
rem default, and the winget copy lands under a long generated name.
set "MINGW="
where g++ >nul 2>&1 && for /f "delims=" %%p in ('where g++') do set "MINGW=%%~dpp"

if not defined MINGW (
    for /d %%d in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.*") do (
        if exist "%%d\mingw64\bin\g++.exe" set "MINGW=%%d\mingw64\bin\"
    )
)
if not defined MINGW if exist "C:\w64devkit\bin\g++.exe" set "MINGW=C:\w64devkit\bin\"
if not defined MINGW if exist "C:\msys64\mingw64\bin\g++.exe" set "MINGW=C:\msys64\mingw64\bin\"
if not defined MINGW if exist "C:\mingw64\bin\g++.exe" set "MINGW=C:\mingw64\bin\"

if defined MINGW goto haveMingw
echo A mingw-w64 compiler was not found. Install one with:
echo     winget install --id BrechtSanders.WinLibs.POSIX.UCRT
echo.
pause
exit /b 1

:haveMingw

rem WinLibs and w64devkit both ship mingw32-make rather than make, so looking
rem only for the latter finds nothing on a perfectly good toolchain.
set "MAKE="
if exist "!MINGW!mingw32-make.exe" set "MAKE=!MINGW!mingw32-make.exe"
if not defined MAKE if exist "!MINGW!make.exe" set "MAKE=!MINGW!make.exe"

if defined MAKE goto haveMake
echo Neither mingw32-make nor make was found beside the compiler in:
echo     !MINGW!
echo.
pause
exit /b 1

:haveMake

set "PATH=!MINGW!;%PATH%"

echo ==========================================================
echo   Building the engine
echo ==========================================================
echo.
echo   cmake    : %CMAKE%
echo   compiler : !MINGW!
echo   make     : !MAKE!
echo.

rem A cache left by another generator blocks the configure step, so drop it.
if exist build\CMakeCache.txt (
    findstr /c:"CMAKE_GENERATOR:INTERNAL=Unix Makefiles" build\CMakeCache.txt >nul 2>&1
    if errorlevel 1 (
        echo   clearing a cache left by another generator
        del /q build\CMakeCache.txt >nul 2>&1
        rmdir /s /q build\CMakeFiles >nul 2>&1
    )
)

rem Unix Makefiles rather than MinGW Makefiles: the latter refuses to run
rem while sh.exe is on the PATH, which it is wherever Git is installed.
rem CMAKE_POLICY_VERSION_MINIMUM: Libraries/doctest asks for CMake 3.0, which
rem CMake 4.x rejects without it.
"%CMAKE%" -S . -B build -G "Unix Makefiles" ^
    -DCMAKE_MAKE_PROGRAM="!MAKE:\=/!" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
if errorlevel 1 goto failed

"%CMAKE%" --build build -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 goto buildFailed

echo.
echo ==========================================================
echo   Checking it
echo ==========================================================
echo.
build\bin\UnitTests.exe
if errorlevel 1 goto failed

echo.
echo ==========================================================
echo   Built. Run train.bat next.
echo ==========================================================
echo.
pause
exit /b 0

:buildFailed
echo.
if exist build\bin\libconquer-the-spire.dll (
    echo   If that was a permission denied on libconquer-the-spire.dll, then
    echo   something still has it open - a training window, or a python. Close
    echo   them and run this again. Watch for it: the static library builds
    echo   anyway, so the only sign is that Python still sees the old engine.
    echo.
)

:failed
echo   The build did not finish.
echo.
pause
exit /b 1

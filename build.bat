@echo off
REM build.bat — Windows build script for Lights Out Christmas
REM Usage:
REM   build.bat              - Release build
REM   build.bat debug        - Debug build
REM   build.bat test         - Build and run tests
REM   build.bat clean        - Remove build directory

setlocal

set BUILD_TYPE=Release
if "%1"=="debug" set BUILD_TYPE=Debug
if "%1"=="clean" goto :clean

set BUILD_DIR=build-%BUILD_TYPE%

echo === Lights Out Christmas ===
echo Build type: %BUILD_TYPE%
echo Build dir:  %BUILD_DIR%
echo.

cmake -B "%BUILD_DIR%" ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_CXX_COMPILER=clang++ ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -G "Ninja" ^
    .

if errorlevel 1 goto :error

cmake --build "%BUILD_DIR%" --parallel
if errorlevel 1 goto :error

if "%1"=="test" (
    echo.
    echo === Running tests ===
    cd "%BUILD_DIR%"
    ctest --output-on-failure
    cd ..
)

echo.
echo === Build complete ===
echo Executable: %BUILD_DIR%\bin\LightsOutChristmas.exe
goto :end

:clean
echo Removing build directories...
if exist build-Release rmdir /s /q build-Release
if exist build-Debug rmdir /s /q build-Debug
echo Done.
goto :end

:error
echo BUILD FAILED
exit /b 1

:end
endlocal

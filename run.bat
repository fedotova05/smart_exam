@echo off
chcp 65001 >nul
cd /d "%~dp0"
if not defined SFML_DIR set "SFML_DIR=C:\SFML-2.5.1"
if not defined MINGW_DIR set "MINGW_DIR=C:\Program Files\CodeBlocks\MinGW"
set "PATH=%SFML_DIR%\bin;%MINGW_DIR%\bin;%PATH%"
if "%1"=="tests" (
    bin\Debug\tests.exe
) else if "%1"=="demo" (
    bin\Debug\smart_exam.exe --demo
) else (
    bin\Debug\smart_exam.exe
)
exit /b %errorlevel%

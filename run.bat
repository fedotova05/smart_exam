@echo off
chcp 65001 >nul
cd /d "%~dp0"
if not defined SFML_DIR set "SFML_DIR=C:\SFML-2.5.1"
if not defined MINGW_DIR set "MINGW_DIR=C:\Program Files\CodeBlocks\MinGW"
set "PATH=%SFML_DIR%\bin;%MINGW_DIR%\bin;%PATH%"
set "SMARTEXAM_EXE=bin\Release\smart_exam.exe"
if not exist "%SMARTEXAM_EXE%" set "SMARTEXAM_EXE=bin\Debug\smart_exam.exe"
if "%1"=="tests" (
    bin\Debug\tests.exe
    if errorlevel 1 exit /b 1
    bin\Debug\ui_tests.exe
) else if "%1"=="demo" (
    "%SMARTEXAM_EXE%" --demo
) else if "%1"=="widgets" (
    "%SMARTEXAM_EXE%" --widgets
) else (
    "%SMARTEXAM_EXE%"
)
exit /b %errorlevel%

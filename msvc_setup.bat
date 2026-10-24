@echo off
@REM Setup MSVC PATH for B:\VS\BuildTools

@REM Unmount any previous virtual drives
subst x: /d >nul 2>&1
subst y: /d >nul 2>&1
subst l: /d >nul 2>&1

@REM MSVC root folder (B:\VS\BuildTools)
for /d %%i in ("B:\VS\BuildTools\VC\Tools\MSVC\*") do (
    subst x: "%%i"
)

@REM Windows SDK Include folder
for /d %%i in ("C:\Program Files (x86)\Windows Kits\10\Include\*") do (
    subst y: "%%i"
)

@REM Windows SDK Lib folder
for /d %%i in ("C:\Program Files (x86)\Windows Kits\10\Lib\*") do (
    subst l: "%%i"
)
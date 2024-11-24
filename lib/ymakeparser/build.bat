@echo off
SetLocal EnableDelayedExpansion

set outputAssembly=ymakep
set compiler=clang++

rem set target=-target x86_64-pc-windows-gnu 

set compilerFlags=-g -Wvarargs -Wall -Werror -std=c++17
set includeFlags=-I./include -I./src

echo ------------------------------
echo C++ build script for %outputAssembly%
echo      by Yassin Ibrahim (Y)
echo ------------------------------
echo.

if not exist "./bin/" (
    mkdir bin
)

if not exist "./obj/" (
    mkdir obj
)

set defines=
rem -DYDEBUG

set srcDir=src
set srcPath=%cd%/src
set objDir=obj

rem calculate the length of the directory(in chars) until srcDir

set length=0

:loop
if not "!srcPath:~%length%,1!"=="" (
    set /a length+=1
    goto loop
) else (
    goto end
)

:end

set lenSrc=0

:loop2
if not "!srcDir:~%lenSrc%, 1!"=="" (
    set /a lenSrc+=1
    goto loop2
) else (
    goto end2
)

:end2

set /a length-=%lenSrc%

rem ___________________________________________________________________________________

set cppFiles=

pushd %srcDir%

for /r %%f in (*.cpp) do (
    set filepath=%%~f

    set "cppFiles=!cppFiles! !filePath:~%length%!"
)

if defined cppFiles set cppFiles=!cppFiles:~1!

echo C++ files to compile =^>

for %%i in (%cppFiles%) do (
    rem trick to sleep for milliseconds
    ping 127.0.0.1 -n 1 -w 100 > nul
    echo %%i
)
echo ----------------------------
echo.

popd

echo compiling source code...

rem -I./thirdparty/include/

%compiler% %target% %cppFiles% %compilerFlags% %includeFlags% %defines% -o bin/%outputAssembly%.exe

echo linking...

rem Check if the compilation was successful
if %ERRORLEVEL% equ 0 (
    echo compilation/linking successful.
) else (
    echo compilation/linking failed.
    exit
)

exit
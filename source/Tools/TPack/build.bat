setlocal

REM
REM  Compiler Setup
REM

REM   Need to set a path to a directory that is relative to this batch file.
REM   use pathenv to output a simple batch file that resolves the relative
REM   path name into a full path name.  Run that batch file to set the
REM   environment variable (the variable to be set is in the pathenv param list)

bin\pathenv temp.bat MSDevDir msdev60
call temp.bat
erase temp.bat

if "%OS%" == "Windows_NT" set PATH=%MSDevDir%\BIN;%MASMDir%\BIN;%MSDevDir%\BIN\%VcOsDir%;%PATH%
if "%OS%" == "" set PATH="%MSDevDir%\BIN";%MASMDir%\BIN;"%MSDevDir%\BIN\%VcOsDir%";"%PATH%"
set INCLUDE=%DXDir%\sdk\inc;%MSDevDir%\INCLUDE;%MSDevDir%\MFC\INCLUDE;%INCLUDE%
set LIB=%DXDir%\sdk\lib;%MSDevDir%\LIB;%MSDevDir%\MFC\LIB;%LIB%
set MSDevDir=
set DXDir=
set MASMDir=
echo %INCLUDE%

REM
REM  Actual Build
REM
rem .\bin\redirect -o stdout.log -e stderr.log .\bin\nmake /f art.mak 
rem if errorlevel 1 goto fail

nmake -f TPack.mak CFG="TPack - Win32 Debug" >> tpackd.txt
nmake -f TPack.mak CFG="TPack - Win32 Release" >> tpackr.txt

if not exist .\release\tpack.exe goto fail
if not exist .\debug\tpack.exe goto fail

if [%1] == [] goto end
echo SUCCESS > %1
goto end

:fail
if [%1] == [] goto end
echo FAILURE > %1

:end


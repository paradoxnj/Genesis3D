echo off
REM   
REM   There is one parameter allowed.  It can be:
REM   CLEAN: to erase all intermediate and target files
REM   DEBUG: builds ONLY the debug versions
REM   RELEASE:  builds ONLY the release versions

set BUILD_FAILURE=


REM   Setup paths for the c compiler and it's tools.
REM   (many of the tools within each target's makefile
REM     are referenced via a relative path too)

REM   only set up the paths if they haven't already been setup. 
if "%PATHS_SET%"== "Yes" GOTO SKIP_PATHS

REM   Need to set a path to a directory that is relative to this batch file.
REM   use pathenv to output a simple batch file that resolves the relative
REM   path name into a full path name.  Run that batch file to set the
REM   environment variable (the variable to be set is in the pathenv param list)

bin\pathenv temp.bat MSDevDir msdev
call temp.bat
erase temp.bat
rem if errorlevel 1 goto BUILD_FAILURE

if "%OS%" == "Windows_NT" set PATH=%MSDevDir%\BIN;%MASMDir%\BIN;%MSDevDir%\BIN\%VcOsDir%;%PATH%
if "%OS%" == "" set PATH="%MSDevDir%\BIN";%MASMDir%\BIN;"%MSDevDir%\BIN\%VcOsDir%";"%PATH%"
set INCLUDE=%DXDir%\sdk\inc;%MSDevDir%\INCLUDE;%MSDevDir%\MFC\INCLUDE;%INCLUDE%
set LIB=%DXDir%\sdk\lib;%MSDevDir%\LIB;%MSDevDir%\MFC\LIB;%LIB%
set MSDevDir=
set DXDir=
set MASMDir=
Set PATHS_SET=Yes
echo %INCLUDE%
REM goto VERY_END
:SKIP_PATHS

REM   Paths are setup.  


REM   Clean up previous build logs.
if exist buildlog\build.log goto BUILDLOG_EXISTS
mkdir buildlog
:BUILDLOG_EXISTS
echo building: > buildlog\build.log
for %%1 in (buildlog\*.*) do del %%1
echo building: > buildlog\build.log

echo Building Debug Version: >> buildlog\build.log
nmake -f gbsplib.mak CFG="GBSPLib - Win32 Debug" >> buildlog\build.log
echo Building Release Version: >> buildlog\build.log
nmake -f gbsplib.mak CFG="GBSPLib - Win32 Release" >> buildlog\build.log


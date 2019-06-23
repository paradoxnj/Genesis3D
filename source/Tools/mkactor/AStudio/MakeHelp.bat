@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by ASTUDIO.HPJ. >"AStudio\hlp\AStudio.hm"
echo. >>"AStudio\hlp\AStudio.hm"
echo // Commands (ID_* and IDM_*) >>"AStudio\hlp\AStudio.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"AStudio\hlp\AStudio.hm"
echo. >>"AStudio\hlp\AStudio.hm"
echo // Prompts (IDP_*) >>"AStudio\hlp\AStudio.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"AStudio\hlp\AStudio.hm"
echo. >>"AStudio\hlp\AStudio.hm"
echo // Resources (IDR_*) >>"AStudio\hlp\AStudio.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"AStudio\hlp\AStudio.hm"
echo. >>"AStudio\hlp\AStudio.hm"
echo // Dialogs (IDD_*) >>"AStudio\hlp\AStudio.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"AStudio\hlp\AStudio.hm"
echo. >>"AStudio\hlp\AStudio.hm"
echo // Frame Controls (IDW_*) >>"AStudio\hlp\AStudio.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"AStudio\hlp\AStudio.hm"
REM -- Make help for Project ASTUDIO


echo Building Win32 Help files
start /wait hcw /C /E /M "AStudio\hlp\AStudio.hpj"
if errorlevel 1 goto :Error
if not exist "AStudio\hlp\AStudio.hlp" goto :Error
if not exist "AStudio\hlp\AStudio.cnt" goto :Error
echo.
if exist Debug\AStudio\nul copy "AStudio\hlp\AStudio.hlp" Debug\AStudio
if exist Debug\AStudio\nul copy "AStudio\hlp\AStudio.cnt" Debug\AStudio
if exist Release\nul copy "AStudio\hlp\AStudio.hlp" Release
if exist Release\nul copy "AStudio\hlp\AStudio.cnt" Release
echo.
goto :done

:Error
echo AStudio\hlp\AStudio.hpj(1) : error: Problem encountered creating help file

:done
echo.

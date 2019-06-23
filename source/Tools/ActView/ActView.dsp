# Microsoft Developer Studio Project File - Name="ActView" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ActView - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ActView.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ActView.mak" CFG="ActView - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ActView - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ActView - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Genesis10/Tools/Actview", ASFBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ActView - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /MT /W4 /GX /O2 /X /I ".\Util" /I ".\Main" /I ".\GenesisSDK\Include" /I "..\..\msdev60\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "WIN32_LEAN_AND_MEAN" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /x /i "..\..\msdev60\include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 genesis.lib libcmt.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib oldnames.lib winmm.lib comctl32.lib ole32.lib uuid.lib urlmon.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib /libpath:".\GenesisSDK\lib" /libpath:"..\..\msdev60\lib"

!ELSEIF  "$(CFG)" == "ActView - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /MTd /W4 /Gm /GX /ZI /Od /X /I ".\Util" /I ".\Main" /I ".\GenesisSDK\Include" /I "..\..\msdev60\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "WIN32_LEAN_AND_MEAN" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /x /i "..\..\msdev60\include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 genesisd.lib libcmtd.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib oldnames.lib winmm.lib comctl32.lib ole32.lib uuid.lib urlmon.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib /pdbtype:sept /libpath:".\GenesisSDK\lib" /libpath:"..\..\msdev60\lib"

!ENDIF 

# Begin Target

# Name "ActView - Win32 Release"
# Name "ActView - Win32 Debug"
# Begin Group "Util"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Util\About.c
# End Source File
# Begin Source File

SOURCE=.\Util\about.h
# End Source File
# Begin Source File

SOURCE=.\Util\drvlist.c
# End Source File
# Begin Source File

SOURCE=.\Util\drvlist.h
# End Source File
# Begin Source File

SOURCE=.\Util\FilePath.c
# End Source File
# Begin Source File

SOURCE=.\Util\FilePath.h
# End Source File
# Begin Source File

SOURCE=.\Util\InstCheck.c
# End Source File
# Begin Source File

SOURCE=.\Util\InstCheck.h
# End Source File
# Begin Source File

SOURCE=.\Util\rcstring.c
# End Source File
# Begin Source File

SOURCE=.\Util\rcstring.h
# End Source File
# Begin Source File

SOURCE=.\Util\units.h
# End Source File
# Begin Source File

SOURCE=.\Util\WinUtil.c
# End Source File
# Begin Source File

SOURCE=.\Util\WinUtil.h
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter ".bmp;.cur;.ico"
# Begin Source File

SOURCE=.\res\actview.bmp
# End Source File
# Begin Source File

SOURCE=.\res\blend.ico
# End Source File
# Begin Source File

SOURCE=.\res\ffend.ico
# End Source File
# Begin Source File

SOURCE=.\res\ffframe.ico
# End Source File
# Begin Source File

SOURCE=.\res\mainicon.ico
# End Source File
# Begin Source File

SOURCE=.\res\pan.cur
# End Source File
# Begin Source File

SOURCE=.\res\pan.ico
# End Source File
# Begin Source File

SOURCE=.\res\pause.ico
# End Source File
# Begin Source File

SOURCE=.\res\play.ico
# End Source File
# Begin Source File

SOURCE=.\res\rotate.cur
# End Source File
# Begin Source File

SOURCE=.\res\rotate.ico
# End Source File
# Begin Source File

SOURCE=.\res\rrframe.ico
# End Source File
# Begin Source File

SOURCE=.\res\rrstart.ico
# End Source File
# Begin Source File

SOURCE=.\res\stop.ico
# End Source File
# Begin Source File

SOURCE=.\res\zoom.cur
# End Source File
# Begin Source File

SOURCE=.\res\zoom.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\Main\actview.c
# End Source File
# Begin Source File

SOURCE=.\ActView.mak
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\Main\actview.rc
# End Source File
# Begin Source File

SOURCE=.\Main\Blender.c
# End Source File
# Begin Source File

SOURCE=.\Main\Blender.h
# End Source File
# Begin Source File

SOURCE=.\Main\resource.h
# End Source File
# End Target
# End Project

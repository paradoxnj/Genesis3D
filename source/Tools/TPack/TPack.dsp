# Microsoft Developer Studio Project File - Name="TPack" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=TPack - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TPack.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TPack.mak" CFG="TPack - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TPack - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "TPack - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Blender", ASFBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TPack - Win32 Release"

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
# ADD CPP /nologo /G5 /MT /W4 /GX /O2 /I ".\Util" /I ".\Source" /I ".\GenesisSDK\Include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "WIN32_LEAN_AND_MEAN" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 genesis.lib libcmt.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib oldnames.lib winmm.lib comctl32.lib ole32.lib uuid.lib urlmon.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib /libpath:".\GenesisSDK\lib"

!ELSEIF  "$(CFG)" == "TPack - Win32 Debug"

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
# ADD CPP /nologo /G5 /MTd /W4 /Gm /GX /ZI /Od /I ".\Util" /I ".\Source" /I ".\GenesisSDK\Include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "WIN32_LEAN_AND_MEAN" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 genesisd.lib libcmtd.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib oldnames.lib winmm.lib comctl32.lib ole32.lib uuid.lib urlmon.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib /pdbtype:sept /libpath:".\GenesisSDK\lib"

!ENDIF 

# Begin Target

# Name "TPack - Win32 Release"
# Name "TPack - Win32 Debug"
# Begin Group "Resources"

# PROP Default_Filter ".bmp;.cur;.ico"
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

SOURCE=.\res\Sourceicon.ico
# End Source File
# Begin Source File

SOURCE=.\res\stop.ico
# End Source File
# Begin Source File

SOURCE=.\res\TPack.bmp
# End Source File
# Begin Source File

SOURCE=.\res\zoom.cur
# End Source File
# Begin Source File

SOURCE=.\res\zoom.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\Source\resource.h
# End Source File
# Begin Source File

SOURCE=.\Source\TPack.c
# End Source File
# Begin Source File

SOURCE=.\Source\TPack.rc
# End Source File
# End Target
# End Project

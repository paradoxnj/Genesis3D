# Microsoft Developer Studio Project File - Name="GBSPLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=GBSPLib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GBSPLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GBSPLib.mak" CFG="GBSPLib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GBSPLib - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "GBSPLib - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Genesis10/Tools/GBSPLib", KTRBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GBSPLib - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GBSPLIB_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /X /I "..\\" /I "SDKShare\Include" /I "..\..\MSDev60\Include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GBSPLIB_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /x /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 /nologo /dll /machine:I386 /nodefaultlib

!ELSEIF  "$(CFG)" == "GBSPLib - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GBSPLIB_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "..\\" /I "SDKShare\Include" /I "..\..\MSDev60\Include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GBSPLIB_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /dll /debug /machine:I386 /nodefaultlib /pdbtype:sept

!ENDIF 

# Begin Target

# Name "GBSPLib - Win32 Release"
# Name "GBSPLib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Brush2.cpp
# End Source File
# Begin Source File

SOURCE=.\Brush2.h
# End Source File
# Begin Source File

SOURCE=.\Bsp.cpp
# End Source File
# Begin Source File

SOURCE=.\Bsp.h
# End Source File
# Begin Source File

SOURCE=.\Bsp2.cpp
# End Source File
# Begin Source File

SOURCE=.\Fill.Cpp
# End Source File
# Begin Source File

SOURCE=.\Fill.h
# End Source File
# Begin Source File

SOURCE=.\Gbspfile.cpp
# End Source File
# Begin Source File

SOURCE=.\Gbspfile.h
# End Source File
# Begin Source File

SOURCE=.\Gbsplib.cpp
# End Source File
# Begin Source File

SOURCE=.\Gbsplib.h
# End Source File
# Begin Source File

SOURCE=.\Gbspprep.cpp
# End Source File
# Begin Source File

SOURCE=.\Gbspprep.h
# End Source File
# Begin Source File

SOURCE=.\Leaf.cpp
# End Source File
# Begin Source File

SOURCE=.\Leaf.h
# End Source File
# Begin Source File

SOURCE=.\Light.cpp
# End Source File
# Begin Source File

SOURCE=.\Light.h
# End Source File
# Begin Source File

SOURCE=.\Map.cpp
# End Source File
# Begin Source File

SOURCE=.\Map.h
# End Source File
# Begin Source File

SOURCE=.\Mathlib.cpp
# End Source File
# Begin Source File

SOURCE=.\Mathlib.h
# End Source File
# Begin Source File

SOURCE=.\Poly.cpp
# End Source File
# Begin Source File

SOURCE=.\Poly.h
# End Source File
# Begin Source File

SOURCE=.\Portals.cpp
# End Source File
# Begin Source File

SOURCE=.\Portals.h
# End Source File
# Begin Source File

SOURCE=.\PortFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Rad.cpp
# End Source File
# Begin Source File

SOURCE=.\Texture.cpp
# End Source File
# Begin Source File

SOURCE=.\Texture.h
# End Source File
# Begin Source File

SOURCE=.\TJunct.cpp
# End Source File
# Begin Source File

SOURCE=.\Utils.cpp
# End Source File
# Begin Source File

SOURCE=.\Utils.h
# End Source File
# Begin Source File

SOURCE=.\Vis.cpp
# End Source File
# Begin Source File

SOURCE=.\Vis.h
# End Source File
# Begin Source File

SOURCE=.\Visflood.cpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Libraries"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\SDKShare\Lib\genesisd.lib

!IF  "$(CFG)" == "GBSPLib - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "GBSPLib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SDKShare\Lib\genesis.lib

!IF  "$(CFG)" == "GBSPLib - Win32 Release"

!ELSEIF  "$(CFG)" == "GBSPLib - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\MSDev60\lib\Winspool.lib
# End Source File
# Begin Source File

SOURCE=..\..\MSDev60\lib\Comdlg32.lib
# End Source File
# Begin Source File

SOURCE=..\..\MSDev60\lib\Gdi32.lib
# End Source File
# Begin Source File

SOURCE=..\..\MSDev60\lib\Kernel32.lib
# End Source File
# Begin Source File

SOURCE=..\..\MSDev60\lib\Libcmt.lib

!IF  "$(CFG)" == "GBSPLib - Win32 Release"

!ELSEIF  "$(CFG)" == "GBSPLib - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\MSDev60\lib\Libcmtd.lib

!IF  "$(CFG)" == "GBSPLib - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "GBSPLib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\MSDev60\lib\Oldnames.lib
# End Source File
# Begin Source File

SOURCE=..\..\MSDev60\lib\Shell32.lib
# End Source File
# Begin Source File

SOURCE=..\..\MSDev60\lib\User32.lib
# End Source File
# Begin Source File

SOURCE=..\..\MSDev60\lib\Uuid.lib
# End Source File
# Begin Source File

SOURCE=..\..\MSDev60\lib\Advapi32.lib
# End Source File
# End Group
# End Target
# End Project

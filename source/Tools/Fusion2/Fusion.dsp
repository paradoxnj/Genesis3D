# Microsoft Developer Studio Project File - Name="FUSION" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=FUSION - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Fusion.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Fusion.mak" CFG="FUSION - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FUSION - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "FUSION - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Fusion2", UNHAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "FUSION - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W4 /GX /Ot /Oa /X /I "." /I ".\GenesisSDK\include" /I ".\TypeParser" /I "..\..\msdev60\include" /I "..\..\msdev60\mfc\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "WIN32_LEAN_AND_MEAN" /YX"stdafx.h" /FD /c
# SUBTRACT CPP /Ox /Og /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /x /i "..\..\msdev60\include" /i "..\..\msdev60\mfc\include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 nafxcw.lib libcmt.lib .\GenesisSDK\lib\genesis.lib advapi32.lib ctl3d32.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib oldnames.lib ole32.lib oleaut32.lib oledlg.lib shell32.lib urlmon.lib user32.lib uuid.lib winmm.lib winspool.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib /libpath:"..\..\msdev60\lib" /libpath:"..\..\msdev60\mfc\lib"
# SUBTRACT LINK32 /incremental:yes /debug

!ELSEIF  "$(CFG)" == "FUSION - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W4 /Gm /Gi /GX /ZI /Od /X /I "." /I ".\GenesisSDK\include" /I ".\TypeParser" /I "..\..\msdev60\include" /I "..\..\msdev60\mfc\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "WIN32_LEAN_AND_MEAN" /YX"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /x /i "..\..\msdev60\include" /i "..\..\msdev60\mfc\include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 nafxcwd.lib libcmtd.lib .\GenesisSDK\lib\genesisd.lib advapi32.lib ctl3d32.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib oldnames.lib ole32.lib oleaut32.lib oledlg.lib shell32.lib urlmon.lib user32.lib uuid.lib winmm.lib winspool.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libcd.lib" /nodefaultlib /libpath:"..\..\msdev60\lib" /libpath:"..\..\msdev60\mfc\lib"
# SUBTRACT LINK32 /incremental:no

!ENDIF 

# Begin Target

# Name "FUSION - Win32 Release"
# Name "FUSION - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Group "TypeParser"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\TypeParser\cparser.c

!IF  "$(CFG)" == "FUSION - Win32 Release"

!ELSEIF  "$(CFG)" == "FUSION - Win32 Debug"

# ADD CPP /I "..\..\msdev60\include" /I "..\..\msdev60\mfc\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TypeParser\cparser.h
# End Source File
# Begin Source File

SOURCE=.\TypeParser\cscanner.c
# End Source File
# Begin Source File

SOURCE=.\TypeParser\cscanner.h
# End Source File
# Begin Source File

SOURCE=.\TypeParser\Hash.c
# End Source File
# Begin Source File

SOURCE=.\TypeParser\Hash.h
# End Source File
# Begin Source File

SOURCE=.\TypeParser\iden.C
# End Source File
# Begin Source File

SOURCE=.\TypeParser\iden.H
# End Source File
# Begin Source File

SOURCE=.\TypeParser\scanner.c
# End Source File
# Begin Source File

SOURCE=.\TypeParser\scanner.h
# End Source File
# Begin Source File

SOURCE=.\TypeParser\symtab.c
# End Source File
# Begin Source File

SOURCE=.\TypeParser\symtab.h
# End Source File
# Begin Source File

SOURCE=.\TypeParser\type.c
# End Source File
# Begin Source File

SOURCE=.\TypeParser\type.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\about.cpp
# End Source File
# Begin Source File

SOURCE=.\ActivationWatch.cpp
# End Source File
# Begin Source File

SOURCE=.\array.c
# End Source File
# Begin Source File

SOURCE=.\box3d.c
# End Source File
# Begin Source File

SOURCE=.\brush.c
# End Source File
# Begin Source File

SOURCE=.\BrushAttributesDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\BrushEntityDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\BrushGroupDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\BrushTemplate.c
# End Source File
# Begin Source File

SOURCE=.\ChildFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\ColorBtn.cpp
# End Source File
# Begin Source File

SOURCE=.\CompileDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Compiler.cpp
# End Source File
# Begin Source File

SOURCE=.\ConsoleTab.cpp
# End Source File
# Begin Source File

SOURCE=.\CreateArchDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\CreateBoxDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\CreateConeDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\CreateCylDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\CreateSpheroidDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\CreateStaircaseDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\EntitiesDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Entity.cpp
# End Source File
# Begin Source File

SOURCE=.\EntityTable.cpp
# End Source File
# Begin Source File

SOURCE=.\EntityVisDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\EntTypeName.c
# End Source File
# Begin Source File

SOURCE=.\EntView.cpp
# End Source File
# Begin Source File

SOURCE=.\face.c
# End Source File
# Begin Source File

SOURCE=.\FaceAttributesDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\FaceList.c
# End Source File
# Begin Source File

SOURCE=.\FilePath.c
# End Source File
# Begin Source File

SOURCE=.\FUSION.cpp
# End Source File
# Begin Source File

SOURCE=.\FUSION.rc
# End Source File
# Begin Source File

SOURCE=.\FUSIONDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\FusionTabControls.cpp
# End Source File
# Begin Source File

SOURCE=.\FUSIONView.cpp
# End Source File
# Begin Source File

SOURCE=.\GridSizeDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\group.cpp
# End Source File
# Begin Source File

SOURCE=.\KeyEditDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\level.cpp
# End Source File
# Begin Source File

SOURCE=.\LevelOptions.cpp
# End Source File
# Begin Source File

SOURCE=.\List.c
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\model.c
# End Source File
# Begin Source File

SOURCE=.\ModelDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\node.c
# End Source File
# Begin Source File

SOURCE=.\Parse3dt.c
# End Source File
# Begin Source File

SOURCE=.\PreferencesDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Prefs.c
# End Source File
# Begin Source File

SOURCE=.\render.c
# End Source File
# Begin Source File

SOURCE=.\SelBrushList.c
# End Source File
# Begin Source File

SOURCE=.\SelFaceList.c
# End Source File
# Begin Source File

SOURCE=.\SkyDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\stack.c
# End Source File
# Begin Source File

SOURCE=.\TextureDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\typeio.c
# End Source File
# Begin Source File

SOURCE=.\undostack.cpp
# End Source File
# Begin Source File

SOURCE=.\util.c
# End Source File
# Begin Source File

SOURCE=.\WadFile.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\about.h
# End Source File
# Begin Source File

SOURCE=.\ActivationWatch.h
# End Source File
# Begin Source File

SOURCE=.\array.h
# End Source File
# Begin Source File

SOURCE=.\box3d.h
# End Source File
# Begin Source File

SOURCE=.\brush.h
# End Source File
# Begin Source File

SOURCE=.\BrushAttributesDialog.h
# End Source File
# Begin Source File

SOURCE=.\BrushEntityDialog.h
# End Source File
# Begin Source File

SOURCE=.\BrushGroupDialog.h
# End Source File
# Begin Source File

SOURCE=.\BrushTemplate.h
# End Source File
# Begin Source File

SOURCE=.\ChildFrm.h
# End Source File
# Begin Source File

SOURCE=.\ColorBtn.h
# End Source File
# Begin Source File

SOURCE=.\CompileDialog.h
# End Source File
# Begin Source File

SOURCE=.\Compiler.h
# End Source File
# Begin Source File

SOURCE=.\ConsoleTab.h
# End Source File
# Begin Source File

SOURCE=.\CreateArchDialog.h
# End Source File
# Begin Source File

SOURCE=.\CreateBoxDialog.h
# End Source File
# Begin Source File

SOURCE=.\CreateConeDialog.h
# End Source File
# Begin Source File

SOURCE=.\CreateCylDialog.h
# End Source File
# Begin Source File

SOURCE=.\CreateSpheroidDialog.h
# End Source File
# Begin Source File

SOURCE=.\CreateStaircaseDialog.h
# End Source File
# Begin Source File

SOURCE=.\EntitiesDialog.h
# End Source File
# Begin Source File

SOURCE=.\Entity.h
# End Source File
# Begin Source File

SOURCE=.\EntityTable.h
# End Source File
# Begin Source File

SOURCE=.\EntityVisDlg.h
# End Source File
# Begin Source File

SOURCE=.\EntTypeName.h
# End Source File
# Begin Source File

SOURCE=.\EntView.h
# End Source File
# Begin Source File

SOURCE=.\face.h
# End Source File
# Begin Source File

SOURCE=.\FaceAttributesDialog.h
# End Source File
# Begin Source File

SOURCE=.\facelist.h
# End Source File
# Begin Source File

SOURCE=.\FilePath.h
# End Source File
# Begin Source File

SOURCE=.\Fusion.h
# End Source File
# Begin Source File

SOURCE=.\FUSIONDoc.h
# End Source File
# Begin Source File

SOURCE=.\FusionTabControls.h
# End Source File
# Begin Source File

SOURCE=.\Fusionview.h
# End Source File
# Begin Source File

SOURCE=.\Gbsplib.h
# End Source File
# Begin Source File

SOURCE=.\GridSizeDialog.h
# End Source File
# Begin Source File

SOURCE=.\group.h
# End Source File
# Begin Source File

SOURCE=.\KeyEditDlg.h
# End Source File
# Begin Source File

SOURCE=.\level.h
# End Source File
# Begin Source File

SOURCE=.\LevelOptions.h
# End Source File
# Begin Source File

SOURCE=.\list.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\model.h
# End Source File
# Begin Source File

SOURCE=.\ModelDialog.h
# End Source File
# Begin Source File

SOURCE=.\Mydef.h
# End Source File
# Begin Source File

SOURCE=.\node.h
# End Source File
# Begin Source File

SOURCE=.\Parse3dt.h
# End Source File
# Begin Source File

SOURCE=.\PreferencesDialog.h
# End Source File
# Begin Source File

SOURCE=.\prefs.h
# End Source File
# Begin Source File

SOURCE=.\render.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\resource.hm
# End Source File
# Begin Source File

SOURCE=.\SelBrushList.h
# End Source File
# Begin Source File

SOURCE=.\SelFaceList.h
# End Source File
# Begin Source File

SOURCE=.\SkyDialog.h
# End Source File
# Begin Source File

SOURCE=.\stack.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TextureDialog.h
# End Source File
# Begin Source File

SOURCE=.\typeio.h
# End Source File
# Begin Source File

SOURCE=.\undostack.h
# End Source File
# Begin Source File

SOURCE=.\units.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# Begin Source File

SOURCE=.\WadFile.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\actor.ico
# End Source File
# Begin Source File

SOURCE=.\res\arch.ico
# End Source File
# Begin Source File

SOURCE=.\res\arrow.cur
# End Source File
# Begin Source File

SOURCE=.\res\camera.bmp
# End Source File
# Begin Source File

SOURCE=.\res\cone.ico
# End Source File
# Begin Source File

SOURCE=.\res\Cross_m.cur
# End Source File
# Begin Source File

SOURCE=.\res\cube.ico
# End Source File
# Begin Source File

SOURCE=.\res\cylinder.bmp
# End Source File
# Begin Source File

SOURCE=.\res\cylinder.ico
# End Source File
# Begin Source File

SOURCE=.\res\default.bmp
# End Source File
# Begin Source File

SOURCE=.\res\eyedropper.cur
# End Source File
# Begin Source File

SOURCE=.\res\ffend.ico
# End Source File
# Begin Source File

SOURCE=.\res\ffframe.ico
# End Source File
# Begin Source File

SOURCE=.\res\FUSION.ico
# End Source File
# Begin Source File

SOURCE=.\res\FUSION.rc2
# End Source File
# Begin Source File

SOURCE=.\res\FUSIONDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\groupbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\hollowbo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\hollowsp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icoPlay.ico
# End Source File
# Begin Source File

SOURCE=.\res\libobj.ico
# End Source File
# Begin Source File

SOURCE=.\res\light.bmp
# End Source File
# Begin Source File

SOURCE=.\res\light.ico
# End Source File
# Begin Source File

SOURCE=.\res\modelorg.bmp
# End Source File
# Begin Source File

SOURCE=.\res\playstart.ico
# End Source File
# Begin Source File

SOURCE=.\res\pointer.cur
# End Source File
# Begin Source File

SOURCE=.\res\rrframe.ico
# End Source File
# Begin Source File

SOURCE=.\res\rrstart.ico
# End Source File
# Begin Source File

SOURCE=.\res\solidbox.bmp
# End Source File
# Begin Source File

SOURCE=.\res\solidsph.bmp
# End Source File
# Begin Source File

SOURCE=.\res\sphere.ico
# End Source File
# Begin Source File

SOURCE=.\res\spotlight.bmp
# End Source File
# Begin Source File

SOURCE=.\res\stairs.ico
# End Source File
# Begin Source File

SOURCE=.\res\stop.ico
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\toolbar1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Whoosh.wav
# End Source File
# End Group
# Begin Source File

SOURCE=.\Fusion.mak
# End Source File
# End Target
# End Project
# Section FUSION : {37C270D0-4771-11CF-944B-0000E8C4CFB1}
# 	0:8:Splash.h:C:\Fusion\Splash.h
# 	0:10:Splash.cpp:C:\Fusion\Splash.cpp
# 	1:10:IDB_SPLASH:105
# 	2:10:ResHdrName:resource.h
# 	2:11:ProjHdrName:stdafx.h
# 	2:10:WrapperDef:_SPLASH_SCRN_
# 	2:12:SplClassName:CSplashWnd
# 	2:21:SplashScreenInsertKey:4.0
# 	2:10:HeaderName:Splash.h
# 	2:10:ImplemName:Splash.cpp
# 	2:7:BmpID16:IDB_SPLASH
# End Section

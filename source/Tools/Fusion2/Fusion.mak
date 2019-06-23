# Microsoft Developer Studio Generated NMAKE File, Based on Fusion.dsp
!IF "$(CFG)" == ""
CFG=FUSION - Win32 Release
!MESSAGE No configuration specified. Defaulting to FUSION - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "FUSION - Win32 Release" && "$(CFG)" != "FUSION - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "FUSION - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\Fusion.exe"


CLEAN :
	-@erase "$(INTDIR)\about.obj"
	-@erase "$(INTDIR)\ActivationWatch.obj"
	-@erase "$(INTDIR)\array.obj"
	-@erase "$(INTDIR)\box3d.obj"
	-@erase "$(INTDIR)\brush.obj"
	-@erase "$(INTDIR)\BrushAttributesDialog.obj"
	-@erase "$(INTDIR)\BrushEntityDialog.obj"
	-@erase "$(INTDIR)\BrushGroupDialog.obj"
	-@erase "$(INTDIR)\BrushTemplate.obj"
	-@erase "$(INTDIR)\ChildFrm.obj"
	-@erase "$(INTDIR)\ColorBtn.obj"
	-@erase "$(INTDIR)\CompileDialog.obj"
	-@erase "$(INTDIR)\Compiler.obj"
	-@erase "$(INTDIR)\ConsoleTab.obj"
	-@erase "$(INTDIR)\cparser.obj"
	-@erase "$(INTDIR)\CreateArchDialog.obj"
	-@erase "$(INTDIR)\CreateBoxDialog.obj"
	-@erase "$(INTDIR)\CreateConeDialog.obj"
	-@erase "$(INTDIR)\CreateCylDialog.obj"
	-@erase "$(INTDIR)\CreateSpheroidDialog.obj"
	-@erase "$(INTDIR)\CreateStaircaseDialog.obj"
	-@erase "$(INTDIR)\cscanner.obj"
	-@erase "$(INTDIR)\EntitiesDialog.obj"
	-@erase "$(INTDIR)\Entity.obj"
	-@erase "$(INTDIR)\EntityTable.obj"
	-@erase "$(INTDIR)\EntityVisDlg.obj"
	-@erase "$(INTDIR)\EntTypeName.obj"
	-@erase "$(INTDIR)\EntView.obj"
	-@erase "$(INTDIR)\face.obj"
	-@erase "$(INTDIR)\FaceAttributesDialog.obj"
	-@erase "$(INTDIR)\FaceList.obj"
	-@erase "$(INTDIR)\FilePath.obj"
	-@erase "$(INTDIR)\FUSION.obj"
	-@erase "$(INTDIR)\FUSION.res"
	-@erase "$(INTDIR)\FUSIONDoc.obj"
	-@erase "$(INTDIR)\FusionTabControls.obj"
	-@erase "$(INTDIR)\FUSIONView.obj"
	-@erase "$(INTDIR)\GridSizeDialog.obj"
	-@erase "$(INTDIR)\group.obj"
	-@erase "$(INTDIR)\Hash.obj"
	-@erase "$(INTDIR)\iden.obj"
	-@erase "$(INTDIR)\KeyEditDlg.obj"
	-@erase "$(INTDIR)\level.obj"
	-@erase "$(INTDIR)\LevelOptions.obj"
	-@erase "$(INTDIR)\List.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\model.obj"
	-@erase "$(INTDIR)\ModelDialog.obj"
	-@erase "$(INTDIR)\node.obj"
	-@erase "$(INTDIR)\Parse3dt.obj"
	-@erase "$(INTDIR)\PreferencesDialog.obj"
	-@erase "$(INTDIR)\Prefs.obj"
	-@erase "$(INTDIR)\render.obj"
	-@erase "$(INTDIR)\scanner.obj"
	-@erase "$(INTDIR)\SelBrushList.obj"
	-@erase "$(INTDIR)\SelFaceList.obj"
	-@erase "$(INTDIR)\SkyDialog.obj"
	-@erase "$(INTDIR)\stack.obj"
	-@erase "$(INTDIR)\symtab.obj"
	-@erase "$(INTDIR)\TextureDialog.obj"
	-@erase "$(INTDIR)\type.obj"
	-@erase "$(INTDIR)\typeio.obj"
	-@erase "$(INTDIR)\undostack.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\WadFile.obj"
	-@erase "$(OUTDIR)\Fusion.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W4 /GX /Ot /Oa /X /I "." /I ".\GenesisSDK\include" /I ".\TypeParser" /I "..\..\msdev60\include" /I "..\..\msdev60\mfc\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "WIN32_LEAN_AND_MEAN" /Fp"$(INTDIR)\Fusion.pch" /YX"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\FUSION.res" /i "..\..\msdev60\include" /i "..\..\msdev60\mfc\include" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Fusion.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=nafxcw.lib libcmt.lib .\GenesisSDK\lib\genesis.lib advapi32.lib ctl3d32.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib oldnames.lib ole32.lib oleaut32.lib oledlg.lib shell32.lib urlmon.lib user32.lib uuid.lib winmm.lib winspool.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\Fusion.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\Fusion.exe" /libpath:"..\..\msdev60\lib" /libpath:"..\..\msdev60\mfc\lib" 
LINK32_OBJS= \
	"$(INTDIR)\cparser.obj" \
	"$(INTDIR)\cscanner.obj" \
	"$(INTDIR)\Hash.obj" \
	"$(INTDIR)\iden.obj" \
	"$(INTDIR)\scanner.obj" \
	"$(INTDIR)\symtab.obj" \
	"$(INTDIR)\type.obj" \
	"$(INTDIR)\about.obj" \
	"$(INTDIR)\ActivationWatch.obj" \
	"$(INTDIR)\array.obj" \
	"$(INTDIR)\box3d.obj" \
	"$(INTDIR)\brush.obj" \
	"$(INTDIR)\BrushAttributesDialog.obj" \
	"$(INTDIR)\BrushEntityDialog.obj" \
	"$(INTDIR)\BrushGroupDialog.obj" \
	"$(INTDIR)\BrushTemplate.obj" \
	"$(INTDIR)\ChildFrm.obj" \
	"$(INTDIR)\ColorBtn.obj" \
	"$(INTDIR)\CompileDialog.obj" \
	"$(INTDIR)\Compiler.obj" \
	"$(INTDIR)\ConsoleTab.obj" \
	"$(INTDIR)\CreateArchDialog.obj" \
	"$(INTDIR)\CreateBoxDialog.obj" \
	"$(INTDIR)\CreateConeDialog.obj" \
	"$(INTDIR)\CreateCylDialog.obj" \
	"$(INTDIR)\CreateSpheroidDialog.obj" \
	"$(INTDIR)\CreateStaircaseDialog.obj" \
	"$(INTDIR)\EntitiesDialog.obj" \
	"$(INTDIR)\Entity.obj" \
	"$(INTDIR)\EntityTable.obj" \
	"$(INTDIR)\EntityVisDlg.obj" \
	"$(INTDIR)\EntTypeName.obj" \
	"$(INTDIR)\EntView.obj" \
	"$(INTDIR)\face.obj" \
	"$(INTDIR)\FaceAttributesDialog.obj" \
	"$(INTDIR)\FaceList.obj" \
	"$(INTDIR)\FilePath.obj" \
	"$(INTDIR)\FUSION.obj" \
	"$(INTDIR)\FUSIONDoc.obj" \
	"$(INTDIR)\FusionTabControls.obj" \
	"$(INTDIR)\FUSIONView.obj" \
	"$(INTDIR)\GridSizeDialog.obj" \
	"$(INTDIR)\group.obj" \
	"$(INTDIR)\KeyEditDlg.obj" \
	"$(INTDIR)\level.obj" \
	"$(INTDIR)\LevelOptions.obj" \
	"$(INTDIR)\List.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\model.obj" \
	"$(INTDIR)\ModelDialog.obj" \
	"$(INTDIR)\node.obj" \
	"$(INTDIR)\Parse3dt.obj" \
	"$(INTDIR)\PreferencesDialog.obj" \
	"$(INTDIR)\Prefs.obj" \
	"$(INTDIR)\render.obj" \
	"$(INTDIR)\SelBrushList.obj" \
	"$(INTDIR)\SelFaceList.obj" \
	"$(INTDIR)\SkyDialog.obj" \
	"$(INTDIR)\stack.obj" \
	"$(INTDIR)\TextureDialog.obj" \
	"$(INTDIR)\typeio.obj" \
	"$(INTDIR)\undostack.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\WadFile.obj" \
	"$(INTDIR)\FUSION.res"

"$(OUTDIR)\Fusion.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "FUSION - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\Fusion.exe"


CLEAN :
	-@erase "$(INTDIR)\about.obj"
	-@erase "$(INTDIR)\ActivationWatch.obj"
	-@erase "$(INTDIR)\array.obj"
	-@erase "$(INTDIR)\box3d.obj"
	-@erase "$(INTDIR)\brush.obj"
	-@erase "$(INTDIR)\BrushAttributesDialog.obj"
	-@erase "$(INTDIR)\BrushEntityDialog.obj"
	-@erase "$(INTDIR)\BrushGroupDialog.obj"
	-@erase "$(INTDIR)\BrushTemplate.obj"
	-@erase "$(INTDIR)\ChildFrm.obj"
	-@erase "$(INTDIR)\ColorBtn.obj"
	-@erase "$(INTDIR)\CompileDialog.obj"
	-@erase "$(INTDIR)\Compiler.obj"
	-@erase "$(INTDIR)\ConsoleTab.obj"
	-@erase "$(INTDIR)\cparser.obj"
	-@erase "$(INTDIR)\CreateArchDialog.obj"
	-@erase "$(INTDIR)\CreateBoxDialog.obj"
	-@erase "$(INTDIR)\CreateConeDialog.obj"
	-@erase "$(INTDIR)\CreateCylDialog.obj"
	-@erase "$(INTDIR)\CreateSpheroidDialog.obj"
	-@erase "$(INTDIR)\CreateStaircaseDialog.obj"
	-@erase "$(INTDIR)\cscanner.obj"
	-@erase "$(INTDIR)\EntitiesDialog.obj"
	-@erase "$(INTDIR)\Entity.obj"
	-@erase "$(INTDIR)\EntityTable.obj"
	-@erase "$(INTDIR)\EntityVisDlg.obj"
	-@erase "$(INTDIR)\EntTypeName.obj"
	-@erase "$(INTDIR)\EntView.obj"
	-@erase "$(INTDIR)\face.obj"
	-@erase "$(INTDIR)\FaceAttributesDialog.obj"
	-@erase "$(INTDIR)\FaceList.obj"
	-@erase "$(INTDIR)\FilePath.obj"
	-@erase "$(INTDIR)\FUSION.obj"
	-@erase "$(INTDIR)\FUSION.res"
	-@erase "$(INTDIR)\FUSIONDoc.obj"
	-@erase "$(INTDIR)\FusionTabControls.obj"
	-@erase "$(INTDIR)\FUSIONView.obj"
	-@erase "$(INTDIR)\GridSizeDialog.obj"
	-@erase "$(INTDIR)\group.obj"
	-@erase "$(INTDIR)\Hash.obj"
	-@erase "$(INTDIR)\iden.obj"
	-@erase "$(INTDIR)\KeyEditDlg.obj"
	-@erase "$(INTDIR)\level.obj"
	-@erase "$(INTDIR)\LevelOptions.obj"
	-@erase "$(INTDIR)\List.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\model.obj"
	-@erase "$(INTDIR)\ModelDialog.obj"
	-@erase "$(INTDIR)\node.obj"
	-@erase "$(INTDIR)\Parse3dt.obj"
	-@erase "$(INTDIR)\PreferencesDialog.obj"
	-@erase "$(INTDIR)\Prefs.obj"
	-@erase "$(INTDIR)\render.obj"
	-@erase "$(INTDIR)\scanner.obj"
	-@erase "$(INTDIR)\SelBrushList.obj"
	-@erase "$(INTDIR)\SelFaceList.obj"
	-@erase "$(INTDIR)\SkyDialog.obj"
	-@erase "$(INTDIR)\stack.obj"
	-@erase "$(INTDIR)\symtab.obj"
	-@erase "$(INTDIR)\TextureDialog.obj"
	-@erase "$(INTDIR)\type.obj"
	-@erase "$(INTDIR)\typeio.obj"
	-@erase "$(INTDIR)\undostack.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\WadFile.obj"
	-@erase "$(OUTDIR)\Fusion.exe"
	-@erase "$(OUTDIR)\Fusion.ilk"
	-@erase "$(OUTDIR)\Fusion.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W4 /Gm /Gi /GX /ZI /Od /X /I "." /I ".\GenesisSDK\include" /I ".\TypeParser" /I "..\..\msdev60\include" /I "..\..\msdev60\mfc\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "WIN32_LEAN_AND_MEAN" /Fp"$(INTDIR)\Fusion.pch" /YX"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\FUSION.res" /i "..\..\msdev60\include" /i "..\..\msdev60\mfc\include" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Fusion.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=nafxcwd.lib libcmtd.lib .\GenesisSDK\lib\genesisd.lib advapi32.lib ctl3d32.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib oldnames.lib ole32.lib oleaut32.lib oledlg.lib shell32.lib urlmon.lib user32.lib uuid.lib winmm.lib winspool.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\Fusion.pdb" /debug /machine:I386 /nodefaultlib:"libcd.lib" /nodefaultlib /out:"$(OUTDIR)\Fusion.exe" /libpath:"..\..\msdev60\lib" /libpath:"..\..\msdev60\mfc\lib" 
LINK32_OBJS= \
	"$(INTDIR)\cparser.obj" \
	"$(INTDIR)\cscanner.obj" \
	"$(INTDIR)\Hash.obj" \
	"$(INTDIR)\iden.obj" \
	"$(INTDIR)\scanner.obj" \
	"$(INTDIR)\symtab.obj" \
	"$(INTDIR)\type.obj" \
	"$(INTDIR)\about.obj" \
	"$(INTDIR)\ActivationWatch.obj" \
	"$(INTDIR)\array.obj" \
	"$(INTDIR)\box3d.obj" \
	"$(INTDIR)\brush.obj" \
	"$(INTDIR)\BrushAttributesDialog.obj" \
	"$(INTDIR)\BrushEntityDialog.obj" \
	"$(INTDIR)\BrushGroupDialog.obj" \
	"$(INTDIR)\BrushTemplate.obj" \
	"$(INTDIR)\ChildFrm.obj" \
	"$(INTDIR)\ColorBtn.obj" \
	"$(INTDIR)\CompileDialog.obj" \
	"$(INTDIR)\Compiler.obj" \
	"$(INTDIR)\ConsoleTab.obj" \
	"$(INTDIR)\CreateArchDialog.obj" \
	"$(INTDIR)\CreateBoxDialog.obj" \
	"$(INTDIR)\CreateConeDialog.obj" \
	"$(INTDIR)\CreateCylDialog.obj" \
	"$(INTDIR)\CreateSpheroidDialog.obj" \
	"$(INTDIR)\CreateStaircaseDialog.obj" \
	"$(INTDIR)\EntitiesDialog.obj" \
	"$(INTDIR)\Entity.obj" \
	"$(INTDIR)\EntityTable.obj" \
	"$(INTDIR)\EntityVisDlg.obj" \
	"$(INTDIR)\EntTypeName.obj" \
	"$(INTDIR)\EntView.obj" \
	"$(INTDIR)\face.obj" \
	"$(INTDIR)\FaceAttributesDialog.obj" \
	"$(INTDIR)\FaceList.obj" \
	"$(INTDIR)\FilePath.obj" \
	"$(INTDIR)\FUSION.obj" \
	"$(INTDIR)\FUSIONDoc.obj" \
	"$(INTDIR)\FusionTabControls.obj" \
	"$(INTDIR)\FUSIONView.obj" \
	"$(INTDIR)\GridSizeDialog.obj" \
	"$(INTDIR)\group.obj" \
	"$(INTDIR)\KeyEditDlg.obj" \
	"$(INTDIR)\level.obj" \
	"$(INTDIR)\LevelOptions.obj" \
	"$(INTDIR)\List.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\model.obj" \
	"$(INTDIR)\ModelDialog.obj" \
	"$(INTDIR)\node.obj" \
	"$(INTDIR)\Parse3dt.obj" \
	"$(INTDIR)\PreferencesDialog.obj" \
	"$(INTDIR)\Prefs.obj" \
	"$(INTDIR)\render.obj" \
	"$(INTDIR)\SelBrushList.obj" \
	"$(INTDIR)\SelFaceList.obj" \
	"$(INTDIR)\SkyDialog.obj" \
	"$(INTDIR)\stack.obj" \
	"$(INTDIR)\TextureDialog.obj" \
	"$(INTDIR)\typeio.obj" \
	"$(INTDIR)\undostack.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\WadFile.obj" \
	"$(INTDIR)\FUSION.res"

"$(OUTDIR)\Fusion.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("Fusion.dep")
!INCLUDE "Fusion.dep"
!ELSE 
!MESSAGE Warning: cannot find "Fusion.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "FUSION - Win32 Release" || "$(CFG)" == "FUSION - Win32 Debug"
SOURCE=.\TypeParser\cparser.c

!IF  "$(CFG)" == "FUSION - Win32 Release"

CPP_SWITCHES=/nologo /MT /W4 /GX /Ot /Oa /X /I "." /I ".\GenesisSDK\include" /I ".\TypeParser" /I "..\..\msdev60\include" /I "..\..\msdev60\mfc\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "WIN32_LEAN_AND_MEAN" /Fp"$(INTDIR)\Fusion.pch" /YX"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\cparser.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "FUSION - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W4 /Gm /Gi /GX /ZI /Od /X /I "." /I ".\GenesisSDK\include" /I ".\TypeParser" /I "..\..\msdev60\include" /I "..\..\msdev60\mfc\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "WIN32_LEAN_AND_MEAN" /Fp"$(INTDIR)\Fusion.pch" /YX"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\cparser.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\TypeParser\cscanner.c

"$(INTDIR)\cscanner.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\TypeParser\Hash.c

"$(INTDIR)\Hash.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\TypeParser\iden.C

"$(INTDIR)\iden.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\TypeParser\scanner.c

"$(INTDIR)\scanner.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\TypeParser\symtab.c

"$(INTDIR)\symtab.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\TypeParser\type.c

"$(INTDIR)\type.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\about.cpp

"$(INTDIR)\about.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ActivationWatch.cpp

"$(INTDIR)\ActivationWatch.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\array.c

"$(INTDIR)\array.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\box3d.c

"$(INTDIR)\box3d.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\brush.c

"$(INTDIR)\brush.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\BrushAttributesDialog.cpp

"$(INTDIR)\BrushAttributesDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\BrushEntityDialog.cpp

"$(INTDIR)\BrushEntityDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\BrushGroupDialog.cpp

"$(INTDIR)\BrushGroupDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\BrushTemplate.c

"$(INTDIR)\BrushTemplate.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ChildFrm.cpp

"$(INTDIR)\ChildFrm.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ColorBtn.cpp

"$(INTDIR)\ColorBtn.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\CompileDialog.cpp

"$(INTDIR)\CompileDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Compiler.cpp

"$(INTDIR)\Compiler.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ConsoleTab.cpp

"$(INTDIR)\ConsoleTab.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\CreateArchDialog.cpp

"$(INTDIR)\CreateArchDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\CreateBoxDialog.cpp

"$(INTDIR)\CreateBoxDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\CreateConeDialog.cpp

"$(INTDIR)\CreateConeDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\CreateCylDialog.cpp

"$(INTDIR)\CreateCylDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\CreateSpheroidDialog.cpp

"$(INTDIR)\CreateSpheroidDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\CreateStaircaseDialog.cpp

"$(INTDIR)\CreateStaircaseDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\EntitiesDialog.cpp

"$(INTDIR)\EntitiesDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Entity.cpp

"$(INTDIR)\Entity.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\EntityTable.cpp

"$(INTDIR)\EntityTable.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\EntityVisDlg.cpp

"$(INTDIR)\EntityVisDlg.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\EntTypeName.c

"$(INTDIR)\EntTypeName.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\EntView.cpp

"$(INTDIR)\EntView.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\face.c

"$(INTDIR)\face.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\FaceAttributesDialog.cpp

"$(INTDIR)\FaceAttributesDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\FaceList.c

"$(INTDIR)\FaceList.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\FilePath.c

"$(INTDIR)\FilePath.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\FUSION.cpp

"$(INTDIR)\FUSION.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\FUSION.rc

"$(INTDIR)\FUSION.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\FUSIONDoc.cpp

"$(INTDIR)\FUSIONDoc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\FusionTabControls.cpp

"$(INTDIR)\FusionTabControls.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\FUSIONView.cpp

"$(INTDIR)\FUSIONView.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\GridSizeDialog.cpp

"$(INTDIR)\GridSizeDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\group.cpp

"$(INTDIR)\group.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\KeyEditDlg.cpp

"$(INTDIR)\KeyEditDlg.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\level.cpp

"$(INTDIR)\level.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\LevelOptions.cpp

"$(INTDIR)\LevelOptions.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\List.c

"$(INTDIR)\List.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\MainFrm.cpp

"$(INTDIR)\MainFrm.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\model.c

"$(INTDIR)\model.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ModelDialog.cpp

"$(INTDIR)\ModelDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\node.c

"$(INTDIR)\node.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Parse3dt.c

"$(INTDIR)\Parse3dt.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\PreferencesDialog.cpp

"$(INTDIR)\PreferencesDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Prefs.c

"$(INTDIR)\Prefs.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\render.c

"$(INTDIR)\render.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\SelBrushList.c

"$(INTDIR)\SelBrushList.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\SelFaceList.c

"$(INTDIR)\SelFaceList.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\SkyDialog.cpp

"$(INTDIR)\SkyDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\stack.c

"$(INTDIR)\stack.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\TextureDialog.cpp

"$(INTDIR)\TextureDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\typeio.c

"$(INTDIR)\typeio.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\undostack.cpp

"$(INTDIR)\undostack.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\util.c

"$(INTDIR)\util.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\WadFile.cpp

"$(INTDIR)\WadFile.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 


# Microsoft Developer Studio Generated NMAKE File, Based on ActView.dsp
!IF "$(CFG)" == ""
CFG=ActView - Win32 Debug
!MESSAGE No configuration specified. Defaulting to ActView - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ActView - Win32 Release" && "$(CFG)" != "ActView - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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

!IF  "$(CFG)" == "ActView - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\ActView.exe"


CLEAN :
	-@erase "$(INTDIR)\About.obj"
	-@erase "$(INTDIR)\actview.obj"
	-@erase "$(INTDIR)\actview.res"
	-@erase "$(INTDIR)\Blender.obj"
	-@erase "$(INTDIR)\drvlist.obj"
	-@erase "$(INTDIR)\FilePath.obj"
	-@erase "$(INTDIR)\InstCheck.obj"
	-@erase "$(INTDIR)\rcstring.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\WinUtil.obj"
	-@erase "$(OUTDIR)\ActView.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /G5 /MT /W4 /GX /O2 /X /I ".\Util" /I ".\Main" /I ".\GenesisSDK\Include" /I "..\..\msdev60\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "WIN32_LEAN_AND_MEAN" /Fp"$(INTDIR)\ActView.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\actview.res" /i "..\..\msdev60\include" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ActView.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=genesis.lib libcmt.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib oldnames.lib winmm.lib comctl32.lib ole32.lib uuid.lib urlmon.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\ActView.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\ActView.exe" /libpath:".\GenesisSDK\lib" /libpath:"..\..\msdev60\lib" 
LINK32_OBJS= \
	"$(INTDIR)\About.obj" \
	"$(INTDIR)\drvlist.obj" \
	"$(INTDIR)\FilePath.obj" \
	"$(INTDIR)\InstCheck.obj" \
	"$(INTDIR)\rcstring.obj" \
	"$(INTDIR)\WinUtil.obj" \
	"$(INTDIR)\actview.obj" \
	"$(INTDIR)\Blender.obj" \
	"$(INTDIR)\actview.res"

"$(OUTDIR)\ActView.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ActView - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\ActView.exe"


CLEAN :
	-@erase "$(INTDIR)\About.obj"
	-@erase "$(INTDIR)\actview.obj"
	-@erase "$(INTDIR)\actview.res"
	-@erase "$(INTDIR)\Blender.obj"
	-@erase "$(INTDIR)\drvlist.obj"
	-@erase "$(INTDIR)\FilePath.obj"
	-@erase "$(INTDIR)\InstCheck.obj"
	-@erase "$(INTDIR)\rcstring.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\WinUtil.obj"
	-@erase "$(OUTDIR)\ActView.exe"
	-@erase "$(OUTDIR)\ActView.ilk"
	-@erase "$(OUTDIR)\ActView.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /G5 /MTd /W4 /Gm /GX /ZI /Od /X /I ".\Util" /I ".\Main" /I ".\GenesisSDK\Include" /I "..\..\msdev60\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "WIN32_LEAN_AND_MEAN" /Fp"$(INTDIR)\ActView.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\actview.res" /i "..\..\msdev60\include" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ActView.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=genesisd.lib libcmtd.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib oldnames.lib winmm.lib comctl32.lib ole32.lib uuid.lib urlmon.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\ActView.pdb" /debug /machine:I386 /nodefaultlib /out:"$(OUTDIR)\ActView.exe" /pdbtype:sept /libpath:".\GenesisSDK\lib" /libpath:"..\..\msdev60\lib" 
LINK32_OBJS= \
	"$(INTDIR)\About.obj" \
	"$(INTDIR)\drvlist.obj" \
	"$(INTDIR)\FilePath.obj" \
	"$(INTDIR)\InstCheck.obj" \
	"$(INTDIR)\rcstring.obj" \
	"$(INTDIR)\WinUtil.obj" \
	"$(INTDIR)\actview.obj" \
	"$(INTDIR)\Blender.obj" \
	"$(INTDIR)\actview.res"

"$(OUTDIR)\ActView.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("ActView.dep")
!INCLUDE "ActView.dep"
!ELSE 
!MESSAGE Warning: cannot find "ActView.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "ActView - Win32 Release" || "$(CFG)" == "ActView - Win32 Debug"
SOURCE=.\Util\About.c

"$(INTDIR)\About.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Util\drvlist.c

"$(INTDIR)\drvlist.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Util\FilePath.c

"$(INTDIR)\FilePath.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Util\InstCheck.c

"$(INTDIR)\InstCheck.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Util\rcstring.c

"$(INTDIR)\rcstring.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Util\WinUtil.c

"$(INTDIR)\WinUtil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Main\actview.c

"$(INTDIR)\actview.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Main\actview.rc

!IF  "$(CFG)" == "ActView - Win32 Release"


"$(INTDIR)\actview.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /x /fo"$(INTDIR)\actview.res" /i "..\..\msdev60\include" /i "Main" /d "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "ActView - Win32 Debug"


"$(INTDIR)\actview.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /x /fo"$(INTDIR)\actview.res" /i "..\..\msdev60\include" /i "Main" /d "_DEBUG" $(SOURCE)


!ENDIF 

SOURCE=.\Main\Blender.c

"$(INTDIR)\Blender.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 


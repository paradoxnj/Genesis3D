# Microsoft Developer Studio Generated NMAKE File, Based on TPack.dsp
!IF "$(CFG)" == ""
CFG=TPack - Win32 Debug
!MESSAGE No configuration specified. Defaulting to TPack - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "TPack - Win32 Release" && "$(CFG)" != "TPack - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "TPack - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\TPack.exe"


CLEAN :
	-@erase "$(INTDIR)\TPack.obj"
	-@erase "$(INTDIR)\TPack.res"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\TPack.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /G5 /MT /W4 /GX /O2 /I ".\Util" /I ".\Source" /I ".\GenesisSDK\Include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "WIN32_LEAN_AND_MEAN" /Fp"$(INTDIR)\TPack.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\TPack.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\TPack.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=genesis.lib libcmt.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib oldnames.lib winmm.lib comctl32.lib ole32.lib uuid.lib urlmon.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\TPack.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\TPack.exe" /libpath:".\GenesisSDK\lib" 
LINK32_OBJS= \
	"$(INTDIR)\TPack.obj" \
	"$(INTDIR)\TPack.res"

"$(OUTDIR)\TPack.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "TPack - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\TPack.exe"


CLEAN :
	-@erase "$(INTDIR)\TPack.obj"
	-@erase "$(INTDIR)\TPack.res"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\TPack.exe"
	-@erase "$(OUTDIR)\TPack.ilk"
	-@erase "$(OUTDIR)\TPack.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /G5 /MTd /W4 /Gm /GX /ZI /Od /I ".\Util" /I ".\Source" /I ".\GenesisSDK\Include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "WIN32_LEAN_AND_MEAN" /Fp"$(INTDIR)\TPack.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\TPack.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\TPack.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=genesisd.lib libcmtd.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib oldnames.lib winmm.lib comctl32.lib ole32.lib uuid.lib urlmon.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\TPack.pdb" /debug /machine:I386 /nodefaultlib /out:"$(OUTDIR)\TPack.exe" /pdbtype:sept /libpath:".\GenesisSDK\lib" 
LINK32_OBJS= \
	"$(INTDIR)\TPack.obj" \
	"$(INTDIR)\TPack.res"

"$(OUTDIR)\TPack.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("TPack.dep")
!INCLUDE "TPack.dep"
!ELSE 
!MESSAGE Warning: cannot find "TPack.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "TPack - Win32 Release" || "$(CFG)" == "TPack - Win32 Debug"
SOURCE=.\Source\TPack.c

"$(INTDIR)\TPack.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Source\TPack.rc

!IF  "$(CFG)" == "TPack - Win32 Release"


"$(INTDIR)\TPack.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\TPack.res" /i "Source" /d "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "TPack - Win32 Debug"


"$(INTDIR)\TPack.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\TPack.res" /i "Source" /d "_DEBUG" $(SOURCE)


!ENDIF 


!ENDIF 


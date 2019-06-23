# Microsoft Developer Studio Generated NMAKE File, Based on GPreview.dsp
!IF "$(CFG)" == ""
CFG=GPreview - Win32 Debug
!MESSAGE No configuration specified. Defaulting to GPreview - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "GPreview - Win32 Release" && "$(CFG)" != "GPreview - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GPreview.mak" CFG="GPreview - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GPreview - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "GPreview - Win32 Debug" (based on "Win32 (x86) Application")
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

!IF  "$(CFG)" == "GPreview - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\GPreview.exe"


CLEAN :
	-@erase "$(INTDIR)\drvlist.obj"
	-@erase "$(INTDIR)\function.obj"
	-@erase "$(INTDIR)\gpreview.obj"
	-@erase "$(INTDIR)\gpreview.res"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\GPreview.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /X /I "..\..\msdev60\include" /I ".\GenesisSDK\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\GPreview.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\gpreview.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\GPreview.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=genesis.lib libcmt.lib kernel32.lib user32.lib gdi32.lib oldnames.lib winmm.lib urlmon.lib ole32.lib uuid.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\GPreview.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\GPreview.exe" /libpath:".\GenesisSDK\lib" /libpath:"..\..\msdev60\lib" 
LINK32_OBJS= \
	"$(INTDIR)\drvlist.obj" \
	"$(INTDIR)\function.obj" \
	"$(INTDIR)\gpreview.obj" \
	"$(INTDIR)\gpreview.res"

"$(OUTDIR)\GPreview.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "GPreview - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\GPreview.exe"


CLEAN :
	-@erase "$(INTDIR)\drvlist.obj"
	-@erase "$(INTDIR)\function.obj"
	-@erase "$(INTDIR)\gpreview.obj"
	-@erase "$(INTDIR)\gpreview.res"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\GPreview.exe"
	-@erase "$(OUTDIR)\GPreview.ilk"
	-@erase "$(OUTDIR)\GPreview.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W4 /Gm /GX /ZI /Od /X /I "..\..\msdev60\include" /I ".\GenesisSDK\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\GPreview.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\gpreview.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\GPreview.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=genesisd.lib libcmtd.lib kernel32.lib user32.lib gdi32.lib oldnames.lib winmm.lib urlmon.lib ole32.lib uuid.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\GPreview.pdb" /debug /machine:I386 /nodefaultlib /out:"$(OUTDIR)\GPreview.exe" /pdbtype:sept /libpath:".\GenesisSDK\lib" /libpath:"..\..\msdev60\lib" 
LINK32_OBJS= \
	"$(INTDIR)\drvlist.obj" \
	"$(INTDIR)\function.obj" \
	"$(INTDIR)\gpreview.obj" \
	"$(INTDIR)\gpreview.res"

"$(OUTDIR)\GPreview.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("GPreview.dep")
!INCLUDE "GPreview.dep"
!ELSE 
!MESSAGE Warning: cannot find "GPreview.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "GPreview - Win32 Release" || "$(CFG)" == "GPreview - Win32 Debug"
SOURCE=.\drvlist.c

"$(INTDIR)\drvlist.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\function.c

"$(INTDIR)\function.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\gpreview.c

"$(INTDIR)\gpreview.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\gpreview.rc

"$(INTDIR)\gpreview.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 


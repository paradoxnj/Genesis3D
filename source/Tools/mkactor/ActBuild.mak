# Microsoft Developer Studio Generated NMAKE File, Based on ActBuild.dsp
!IF "$(CFG)" == ""
CFG=ActBuild - Win32 Debug
!MESSAGE No configuration specified. Defaulting to ActBuild - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ActBuild - Win32 Release" && "$(CFG)" != "ActBuild - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ActBuild.mak" CFG="ActBuild - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ActBuild - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ActBuild - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ActBuild - Win32 Release"

OUTDIR=.\Release\ActBuild
INTDIR=.\Release\ActBuild
# Begin Custom Macros
OutDir=.\Release\ActBuild
# End Custom Macros

ALL : "$(OUTDIR)\ActBuild.exe"


CLEAN :
	-@erase "$(INTDIR)\ActBuild.obj"
	-@erase "$(INTDIR)\AOptions.obj"
	-@erase "$(INTDIR)\AProject.obj"
	-@erase "$(INTDIR)\Array.obj"
	-@erase "$(INTDIR)\FilePath.obj"
	-@erase "$(INTDIR)\Log.obj"
	-@erase "$(INTDIR)\make.obj"
	-@erase "$(INTDIR)\maxmath.obj"
	-@erase "$(INTDIR)\mkactor.obj"
	-@erase "$(INTDIR)\mkbody.obj"
	-@erase "$(INTDIR)\mkmotion.obj"
	-@erase "$(INTDIR)\mkutil.obj"
	-@erase "$(INTDIR)\mopshell.obj"
	-@erase "$(INTDIR)\mxscript.obj"
	-@erase "$(INTDIR)\pop.obj"
	-@erase "$(INTDIR)\TDBody.obj"
	-@erase "$(INTDIR)\Util.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vph.obj"
	-@erase "$(OUTDIR)\ActBuild.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W4 /GX /O2 /X /I ".\AStudio" /I ".\AStudio\Util" /I ".\GenesisSDK\Include" /I ".\ActBuild" /I ".\common" /I ".\fmtactor" /I ".\mkactor" /I ".\mkbody" /I ".\mkmotion" /I ".\mop" /I "..\..\MsDev60\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "ACTBUILD" /D "WIN32_LEAN_AND_MEAN" /Fp"$(INTDIR)\ActBuild.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ActBuild.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=GenesisSDK\lib\genesis.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\ActBuild.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\ActBuild.exe" 
LINK32_OBJS= \
	"$(INTDIR)\Log.obj" \
	"$(INTDIR)\maxmath.obj" \
	"$(INTDIR)\mkactor.obj" \
	"$(INTDIR)\mkbody.obj" \
	"$(INTDIR)\mkmotion.obj" \
	"$(INTDIR)\mopshell.obj" \
	"$(INTDIR)\pop.obj" \
	"$(INTDIR)\TDBody.obj" \
	"$(INTDIR)\vph.obj" \
	"$(INTDIR)\ActBuild.obj" \
	"$(INTDIR)\AOptions.obj" \
	"$(INTDIR)\AProject.obj" \
	"$(INTDIR)\Array.obj" \
	"$(INTDIR)\FilePath.obj" \
	"$(INTDIR)\make.obj" \
	"$(INTDIR)\mkutil.obj" \
	"$(INTDIR)\mxscript.obj" \
	"$(INTDIR)\Util.obj" \
	"..\..\MSDev60\lib\Winspool.lib" \
	"..\..\MSDev60\lib\Comdlg32.lib" \
	"..\..\MSDev60\lib\Gdi32.lib" \
	"..\..\MSDev60\lib\Kernel32.lib" \
	"..\..\MSDev60\lib\Libcmt.lib" \
	"..\..\MSDev60\lib\Oldnames.lib" \
	"..\..\MSDev60\lib\Shell32.lib" \
	"..\..\MSDev60\lib\User32.lib" \
	"..\..\MSDev60\lib\Uuid.lib" \
	"..\..\MSDev60\lib\Winmm.lib" \
	"..\..\MSDev60\lib\Advapi32.lib" \
	"..\..\MSDev60\lib\Urlmon.lib" \
	"..\..\MSDev60\lib\Ole32.lib"

"$(OUTDIR)\ActBuild.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ActBuild - Win32 Debug"

OUTDIR=.\Debug\ActBuild
INTDIR=.\Debug\ActBuild
# Begin Custom Macros
OutDir=.\Debug\ActBuild
# End Custom Macros

ALL : "$(OUTDIR)\ActBuild.exe"


CLEAN :
	-@erase "$(INTDIR)\ActBuild.obj"
	-@erase "$(INTDIR)\AOptions.obj"
	-@erase "$(INTDIR)\AProject.obj"
	-@erase "$(INTDIR)\Array.obj"
	-@erase "$(INTDIR)\FilePath.obj"
	-@erase "$(INTDIR)\Log.obj"
	-@erase "$(INTDIR)\make.obj"
	-@erase "$(INTDIR)\maxmath.obj"
	-@erase "$(INTDIR)\mkactor.obj"
	-@erase "$(INTDIR)\mkbody.obj"
	-@erase "$(INTDIR)\mkmotion.obj"
	-@erase "$(INTDIR)\mkutil.obj"
	-@erase "$(INTDIR)\mopshell.obj"
	-@erase "$(INTDIR)\mxscript.obj"
	-@erase "$(INTDIR)\pop.obj"
	-@erase "$(INTDIR)\TDBody.obj"
	-@erase "$(INTDIR)\Util.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\vph.obj"
	-@erase "$(OUTDIR)\ActBuild.exe"
	-@erase "$(OUTDIR)\ActBuild.ilk"
	-@erase "$(OUTDIR)\ActBuild.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W4 /Gm /GX /ZI /Od /X /I ".\AStudio" /I ".\AStudio\Util" /I ".\GenesisSDK\Include" /I ".\ActBuild" /I ".\common" /I ".\fmtactor" /I ".\mkactor" /I ".\mkbody" /I ".\mkmotion" /I ".\mop" /I "..\..\MsDev60\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "ACTBUILD" /D "WIN32_LEAN_AND_MEAN" /Fp"$(INTDIR)\ActBuild.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ActBuild.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=GenesisSDK\lib\genesisd.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\ActBuild.pdb" /debug /machine:I386 /nodefaultlib /out:"$(OUTDIR)\ActBuild.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\Log.obj" \
	"$(INTDIR)\maxmath.obj" \
	"$(INTDIR)\mkactor.obj" \
	"$(INTDIR)\mkbody.obj" \
	"$(INTDIR)\mkmotion.obj" \
	"$(INTDIR)\mopshell.obj" \
	"$(INTDIR)\pop.obj" \
	"$(INTDIR)\TDBody.obj" \
	"$(INTDIR)\vph.obj" \
	"$(INTDIR)\ActBuild.obj" \
	"$(INTDIR)\AOptions.obj" \
	"$(INTDIR)\AProject.obj" \
	"$(INTDIR)\Array.obj" \
	"$(INTDIR)\FilePath.obj" \
	"$(INTDIR)\make.obj" \
	"$(INTDIR)\mkutil.obj" \
	"$(INTDIR)\mxscript.obj" \
	"$(INTDIR)\Util.obj" \
	"..\..\MSDev60\lib\Winspool.lib" \
	"..\..\MSDev60\lib\Comdlg32.lib" \
	"..\..\MSDev60\lib\Gdi32.lib" \
	"..\..\MSDev60\lib\Kernel32.lib" \
	"..\..\MSDev60\lib\Libcmtd.lib" \
	"..\..\MSDev60\lib\Oldnames.lib" \
	"..\..\MSDev60\lib\Shell32.lib" \
	"..\..\MSDev60\lib\User32.lib" \
	"..\..\MSDev60\lib\Uuid.lib" \
	"..\..\MSDev60\lib\Winmm.lib" \
	"..\..\MSDev60\lib\Advapi32.lib" \
	"..\..\MSDev60\lib\Urlmon.lib" \
	"..\..\MSDev60\lib\Ole32.lib"

"$(OUTDIR)\ActBuild.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("ActBuild.dep")
!INCLUDE "ActBuild.dep"
!ELSE 
!MESSAGE Warning: cannot find "ActBuild.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "ActBuild - Win32 Release" || "$(CFG)" == "ActBuild - Win32 Debug"
SOURCE=.\mop\Log.c

"$(INTDIR)\Log.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\common\maxmath.c

"$(INTDIR)\maxmath.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\mkactor\mkactor.c

"$(INTDIR)\mkactor.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\mkbody\mkbody.cpp

"$(INTDIR)\mkbody.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\mkmotion\mkmotion.c

"$(INTDIR)\mkmotion.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\mop\mopshell.c

"$(INTDIR)\mopshell.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\mop\pop.c

"$(INTDIR)\pop.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\common\TDBody.c

"$(INTDIR)\TDBody.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\mkbody\vph.c

"$(INTDIR)\vph.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\ActBuild\ActBuild.c

"$(INTDIR)\ActBuild.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\AOptions.c

"$(INTDIR)\AOptions.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\AProject.c

"$(INTDIR)\AProject.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\Util\Array.c

"$(INTDIR)\Array.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\Util\FilePath.c

"$(INTDIR)\FilePath.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\make.c

"$(INTDIR)\make.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\common\mkutil.c

"$(INTDIR)\mkutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\mxscript.c

"$(INTDIR)\mxscript.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\Util\Util.c

"$(INTDIR)\Util.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 


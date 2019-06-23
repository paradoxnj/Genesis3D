# Microsoft Developer Studio Generated NMAKE File, Based on AStudio.dsp
!IF "$(CFG)" == ""
CFG=AStudio - Win32 Debug
!MESSAGE No configuration specified. Defaulting to AStudio - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "AStudio - Win32 Release" && "$(CFG)" != "AStudio - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "AStudio.mak" CFG="AStudio - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "AStudio - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "AStudio - Win32 Debug" (based on "Win32 (x86) Application")
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

!IF  "$(CFG)" == "AStudio - Win32 Release"

OUTDIR=.\Release\AStudio
INTDIR=.\Release\AStudio
# Begin Custom Macros
OutDir=.\Release\AStudio\ 
# End Custom Macros

ALL : "$(OUTDIR)\AStudio.exe"


CLEAN :
	-@erase "$(INTDIR)\AOptions.obj"
	-@erase "$(INTDIR)\AProject.obj"
	-@erase "$(INTDIR)\Array.obj"
	-@erase "$(INTDIR)\AStudio.obj"
	-@erase "$(INTDIR)\AStudio.res"
	-@erase "$(INTDIR)\BodyDlg.obj"
	-@erase "$(INTDIR)\FilePath.obj"
	-@erase "$(INTDIR)\Log.obj"
	-@erase "$(INTDIR)\LogoPage.obj"
	-@erase "$(INTDIR)\make.obj"
	-@erase "$(INTDIR)\MakeHelp.obj"
	-@erase "$(INTDIR)\MaterialsDlg.obj"
	-@erase "$(INTDIR)\maxmath.obj"
	-@erase "$(INTDIR)\mkactor.obj"
	-@erase "$(INTDIR)\mkbody.obj"
	-@erase "$(INTDIR)\mkmotion.obj"
	-@erase "$(INTDIR)\mopshell.obj"
	-@erase "$(INTDIR)\MotionsDlg.obj"
	-@erase "$(INTDIR)\mxscript.obj"
	-@erase "$(INTDIR)\MyFileDlg.obj"
	-@erase "$(INTDIR)\NewPrjDlg.obj"
	-@erase "$(INTDIR)\PathsDlg.obj"
	-@erase "$(INTDIR)\pop.obj"
	-@erase "$(INTDIR)\PropPage.obj"
	-@erase "$(INTDIR)\PropSheet.obj"
	-@erase "$(INTDIR)\Rcstring.obj"
	-@erase "$(INTDIR)\SettingsDlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\TargetDlg.obj"
	-@erase "$(INTDIR)\TDBody.obj"
	-@erase "$(INTDIR)\TextInputDlg.obj"
	-@erase "$(INTDIR)\Util.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vph.obj"
	-@erase "$(OUTDIR)\AStudio.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W4 /GX /O2 /X /I ".\AStudio\Util" /I ".\GenesisSDK\Include" /I ".\common" /I ".\fmtactor" /I ".\mkactor" /I ".\mkbody" /I ".\mkmotion" /I ".\mop" /I "..\..\MsDev60\include" /I "..\..\MsDev60\mfc\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "VC_EXTRALEAN" /Fp"$(INTDIR)\AStudio.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\AStudio.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\AStudio.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=GenesisSDK\lib\genesis.lib ..\..\MSDEV60\MFC\lib\nafxcw.lib ..\..\MSDEV60\lib\libcmt.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\AStudio.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\AStudio.exe" 
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
	"$(INTDIR)\AOptions.obj" \
	"$(INTDIR)\AProject.obj" \
	"$(INTDIR)\Array.obj" \
	"$(INTDIR)\AStudio.obj" \
	"$(INTDIR)\BodyDlg.obj" \
	"$(INTDIR)\FilePath.obj" \
	"$(INTDIR)\LogoPage.obj" \
	"$(INTDIR)\make.obj" \
	"$(INTDIR)\MakeHelp.obj" \
	"$(INTDIR)\MaterialsDlg.obj" \
	"$(INTDIR)\MotionsDlg.obj" \
	"$(INTDIR)\mxscript.obj" \
	"$(INTDIR)\MyFileDlg.obj" \
	"$(INTDIR)\NewPrjDlg.obj" \
	"$(INTDIR)\PathsDlg.obj" \
	"$(INTDIR)\PropPage.obj" \
	"$(INTDIR)\PropSheet.obj" \
	"$(INTDIR)\Rcstring.obj" \
	"$(INTDIR)\SettingsDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\TargetDlg.obj" \
	"$(INTDIR)\TextInputDlg.obj" \
	"$(INTDIR)\Util.obj" \
	"$(INTDIR)\AStudio.res" \
	"..\..\MSDev60\lib\Winspool.lib" \
	"..\..\MSDev60\lib\Comctl32.lib" \
	"..\..\MSDev60\lib\Comdlg32.lib" \
	"..\..\MSDev60\lib\Ctl3d32s.lib" \
	"..\..\MSDev60\lib\Gdi32.lib" \
	"..\..\MSDev60\lib\Kernel32.lib" \
	"..\..\MSDev60\lib\Oldnames.lib" \
	"..\..\MSDev60\lib\Ole32.lib" \
	"..\..\MSDev60\lib\Oleaut32.lib" \
	"..\..\MSDev60\lib\Shell32.lib" \
	"..\..\MSDev60\lib\User32.lib" \
	"..\..\MSDev60\lib\Uuid.lib" \
	"..\..\MSDev60\lib\Winmm.lib" \
	"..\..\MSDev60\lib\Advapi32.lib" \
	"..\..\MSDev60\lib\Urlmon.lib"

"$(OUTDIR)\AStudio.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "AStudio - Win32 Debug"

OUTDIR=.\Debug\AStudio
INTDIR=.\Debug\AStudio
# Begin Custom Macros
OutDir=.\Debug\AStudio\ 
# End Custom Macros

ALL : "$(OUTDIR)\AStudio.exe"


CLEAN :
	-@erase "$(INTDIR)\AOptions.obj"
	-@erase "$(INTDIR)\AProject.obj"
	-@erase "$(INTDIR)\Array.obj"
	-@erase "$(INTDIR)\AStudio.obj"
	-@erase "$(INTDIR)\AStudio.res"
	-@erase "$(INTDIR)\BodyDlg.obj"
	-@erase "$(INTDIR)\FilePath.obj"
	-@erase "$(INTDIR)\Log.obj"
	-@erase "$(INTDIR)\LogoPage.obj"
	-@erase "$(INTDIR)\make.obj"
	-@erase "$(INTDIR)\MakeHelp.obj"
	-@erase "$(INTDIR)\MaterialsDlg.obj"
	-@erase "$(INTDIR)\maxmath.obj"
	-@erase "$(INTDIR)\mkactor.obj"
	-@erase "$(INTDIR)\mkbody.obj"
	-@erase "$(INTDIR)\mkmotion.obj"
	-@erase "$(INTDIR)\mopshell.obj"
	-@erase "$(INTDIR)\MotionsDlg.obj"
	-@erase "$(INTDIR)\mxscript.obj"
	-@erase "$(INTDIR)\MyFileDlg.obj"
	-@erase "$(INTDIR)\NewPrjDlg.obj"
	-@erase "$(INTDIR)\PathsDlg.obj"
	-@erase "$(INTDIR)\pop.obj"
	-@erase "$(INTDIR)\PropPage.obj"
	-@erase "$(INTDIR)\PropSheet.obj"
	-@erase "$(INTDIR)\Rcstring.obj"
	-@erase "$(INTDIR)\SettingsDlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\TargetDlg.obj"
	-@erase "$(INTDIR)\TDBody.obj"
	-@erase "$(INTDIR)\TextInputDlg.obj"
	-@erase "$(INTDIR)\Util.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\vph.obj"
	-@erase "$(OUTDIR)\AStudio.exe"
	-@erase "$(OUTDIR)\AStudio.ilk"
	-@erase "$(OUTDIR)\AStudio.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W4 /Gm /GX /ZI /Od /X /I ".\AStudio\Util" /I ".\GenesisSDK\Include" /I ".\common" /I ".\fmtactor" /I ".\mkactor" /I ".\mkbody" /I ".\mkmotion" /I ".\mop" /I "..\..\MsDev60\include" /I "..\..\MsDev60\mfc\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "WIN32_LEAN_AND_MEAN" /Fp"$(INTDIR)\AStudio.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\AStudio.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\AStudio.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=GenesisSDK\lib\genesisd.lib ..\..\MSDEV60\MFC\lib\nafxcwd.lib ..\..\MSDEV60\lib\libcmtd.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\AStudio.pdb" /debug /machine:I386 /nodefaultlib /out:"$(OUTDIR)\AStudio.exe" /pdbtype:sept 
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
	"$(INTDIR)\AOptions.obj" \
	"$(INTDIR)\AProject.obj" \
	"$(INTDIR)\Array.obj" \
	"$(INTDIR)\AStudio.obj" \
	"$(INTDIR)\BodyDlg.obj" \
	"$(INTDIR)\FilePath.obj" \
	"$(INTDIR)\LogoPage.obj" \
	"$(INTDIR)\make.obj" \
	"$(INTDIR)\MakeHelp.obj" \
	"$(INTDIR)\MaterialsDlg.obj" \
	"$(INTDIR)\MotionsDlg.obj" \
	"$(INTDIR)\mxscript.obj" \
	"$(INTDIR)\MyFileDlg.obj" \
	"$(INTDIR)\NewPrjDlg.obj" \
	"$(INTDIR)\PathsDlg.obj" \
	"$(INTDIR)\PropPage.obj" \
	"$(INTDIR)\PropSheet.obj" \
	"$(INTDIR)\Rcstring.obj" \
	"$(INTDIR)\SettingsDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\TargetDlg.obj" \
	"$(INTDIR)\TextInputDlg.obj" \
	"$(INTDIR)\Util.obj" \
	"$(INTDIR)\AStudio.res" \
	"..\..\MSDev60\lib\Winspool.lib" \
	"..\..\MSDev60\lib\Comctl32.lib" \
	"..\..\MSDev60\lib\Comdlg32.lib" \
	"..\..\MSDev60\lib\Ctl3d32s.lib" \
	"..\..\MSDev60\lib\Gdi32.lib" \
	"..\..\MSDev60\lib\Kernel32.lib" \
	"..\..\MSDev60\lib\Oldnames.lib" \
	"..\..\MSDev60\lib\Ole32.lib" \
	"..\..\MSDev60\lib\Oleaut32.lib" \
	"..\..\MSDev60\lib\Shell32.lib" \
	"..\..\MSDev60\lib\User32.lib" \
	"..\..\MSDev60\lib\Uuid.lib" \
	"..\..\MSDev60\lib\Winmm.lib" \
	"..\..\MSDev60\lib\Advapi32.lib" \
	"..\..\MSDev60\lib\Urlmon.lib"

"$(OUTDIR)\AStudio.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("AStudio.dep")
!INCLUDE "AStudio.dep"
!ELSE 
!MESSAGE Warning: cannot find "AStudio.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "AStudio - Win32 Release" || "$(CFG)" == "AStudio - Win32 Debug"
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


SOURCE=.\AStudio\AOptions.c

"$(INTDIR)\AOptions.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\AProject.c

"$(INTDIR)\AProject.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\Util\Array.c

"$(INTDIR)\Array.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\AStudio.cpp

"$(INTDIR)\AStudio.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\AStudio.rc

!IF  "$(CFG)" == "AStudio - Win32 Release"


"$(INTDIR)\AStudio.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /x /fo"$(INTDIR)\AStudio.res" /i "AStudio" /i "..\..\msdev60\include" /i "..\..\msdev60\mfc\include" /d "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "AStudio - Win32 Debug"


"$(INTDIR)\AStudio.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /x /fo"$(INTDIR)\AStudio.res" /i "AStudio" /i "..\..\msdev60\include" /i "..\..\msdev60\mfc\include" /d "_DEBUG" $(SOURCE)


!ENDIF 

SOURCE=.\AStudio\BodyDlg.cpp

"$(INTDIR)\BodyDlg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\Util\FilePath.c

"$(INTDIR)\FilePath.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\LogoPage.cpp

"$(INTDIR)\LogoPage.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\make.c

"$(INTDIR)\make.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\MakeHelp.cpp

"$(INTDIR)\MakeHelp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\MaterialsDlg.cpp

"$(INTDIR)\MaterialsDlg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\MotionsDlg.cpp

"$(INTDIR)\MotionsDlg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\mxscript.c

"$(INTDIR)\mxscript.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\MyFileDlg.cpp

"$(INTDIR)\MyFileDlg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\NewPrjDlg.cpp

"$(INTDIR)\NewPrjDlg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\PathsDlg.cpp

"$(INTDIR)\PathsDlg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\PropPage.cpp

"$(INTDIR)\PropPage.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\PropSheet.cpp

"$(INTDIR)\PropSheet.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\Util\Rcstring.c

"$(INTDIR)\Rcstring.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\SettingsDlg.cpp

"$(INTDIR)\SettingsDlg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\StdAfx.cpp

"$(INTDIR)\StdAfx.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\TargetDlg.cpp

"$(INTDIR)\TargetDlg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\TextInputDlg.cpp

"$(INTDIR)\TextInputDlg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\AStudio\Util\Util.c

"$(INTDIR)\Util.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 


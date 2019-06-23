# Microsoft Developer Studio Generated NMAKE File, Based on GBSPLib.dsp
!IF "$(CFG)" == ""
CFG=GBSPLib - Win32 Debug
!MESSAGE No configuration specified. Defaulting to GBSPLib - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "GBSPLib - Win32 Release" && "$(CFG)" != "GBSPLib - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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

!IF  "$(CFG)" == "GBSPLib - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\GBSPLib.dll"


CLEAN :
	-@erase "$(INTDIR)\Brush2.obj"
	-@erase "$(INTDIR)\Bsp.obj"
	-@erase "$(INTDIR)\Bsp2.obj"
	-@erase "$(INTDIR)\Fill.obj"
	-@erase "$(INTDIR)\Gbspfile.obj"
	-@erase "$(INTDIR)\Gbsplib.obj"
	-@erase "$(INTDIR)\Gbspprep.obj"
	-@erase "$(INTDIR)\Leaf.obj"
	-@erase "$(INTDIR)\Light.obj"
	-@erase "$(INTDIR)\Map.obj"
	-@erase "$(INTDIR)\Mathlib.obj"
	-@erase "$(INTDIR)\Poly.obj"
	-@erase "$(INTDIR)\Portals.obj"
	-@erase "$(INTDIR)\PortFile.obj"
	-@erase "$(INTDIR)\Rad.obj"
	-@erase "$(INTDIR)\Texture.obj"
	-@erase "$(INTDIR)\TJunct.obj"
	-@erase "$(INTDIR)\Utils.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\Vis.obj"
	-@erase "$(INTDIR)\Visflood.obj"
	-@erase "$(OUTDIR)\GBSPLib.dll"
	-@erase "$(OUTDIR)\GBSPLib.exp"
	-@erase "$(OUTDIR)\GBSPLib.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /X /I "..\\" /I "SDKShare\Include" /I "..\..\MSDev60\Include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GBSPLIB_EXPORTS" /Fp"$(INTDIR)\GBSPLib.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\GBSPLib.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /dll /incremental:no /pdb:"$(OUTDIR)\GBSPLib.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\GBSPLib.dll" /implib:"$(OUTDIR)\GBSPLib.lib" 
LINK32_OBJS= \
	"$(INTDIR)\Brush2.obj" \
	"$(INTDIR)\Bsp.obj" \
	"$(INTDIR)\Bsp2.obj" \
	"$(INTDIR)\Fill.obj" \
	"$(INTDIR)\Gbspfile.obj" \
	"$(INTDIR)\Gbsplib.obj" \
	"$(INTDIR)\Gbspprep.obj" \
	"$(INTDIR)\Leaf.obj" \
	"$(INTDIR)\Light.obj" \
	"$(INTDIR)\Map.obj" \
	"$(INTDIR)\Mathlib.obj" \
	"$(INTDIR)\Poly.obj" \
	"$(INTDIR)\Portals.obj" \
	"$(INTDIR)\PortFile.obj" \
	"$(INTDIR)\Rad.obj" \
	"$(INTDIR)\Texture.obj" \
	"$(INTDIR)\TJunct.obj" \
	"$(INTDIR)\Utils.obj" \
	"$(INTDIR)\Vis.obj" \
	"$(INTDIR)\Visflood.obj" \
	".\SDKShare\Lib\genesis.lib" \
	"..\..\MSDev60\lib\Winspool.lib" \
	"..\..\MSDev60\lib\Comdlg32.lib" \
	"..\..\MSDev60\lib\Gdi32.lib" \
	"..\..\MSDev60\lib\Kernel32.lib" \
	"..\..\MSDev60\lib\Libcmt.lib" \
	"..\..\MSDev60\lib\Oldnames.lib" \
	"..\..\MSDev60\lib\Shell32.lib" \
	"..\..\MSDev60\lib\User32.lib" \
	"..\..\MSDev60\lib\Uuid.lib" \
	"..\..\MSDev60\lib\Advapi32.lib"

"$(OUTDIR)\GBSPLib.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "GBSPLib - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\GBSPLib.dll"


CLEAN :
	-@erase "$(INTDIR)\Brush2.obj"
	-@erase "$(INTDIR)\Bsp.obj"
	-@erase "$(INTDIR)\Bsp2.obj"
	-@erase "$(INTDIR)\Fill.obj"
	-@erase "$(INTDIR)\Gbspfile.obj"
	-@erase "$(INTDIR)\Gbsplib.obj"
	-@erase "$(INTDIR)\Gbspprep.obj"
	-@erase "$(INTDIR)\Leaf.obj"
	-@erase "$(INTDIR)\Light.obj"
	-@erase "$(INTDIR)\Map.obj"
	-@erase "$(INTDIR)\Mathlib.obj"
	-@erase "$(INTDIR)\Poly.obj"
	-@erase "$(INTDIR)\Portals.obj"
	-@erase "$(INTDIR)\PortFile.obj"
	-@erase "$(INTDIR)\Rad.obj"
	-@erase "$(INTDIR)\Texture.obj"
	-@erase "$(INTDIR)\TJunct.obj"
	-@erase "$(INTDIR)\Utils.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\Vis.obj"
	-@erase "$(INTDIR)\Visflood.obj"
	-@erase "$(OUTDIR)\GBSPLib.dll"
	-@erase "$(OUTDIR)\GBSPLib.exp"
	-@erase "$(OUTDIR)\GBSPLib.ilk"
	-@erase "$(OUTDIR)\GBSPLib.lib"
	-@erase "$(OUTDIR)\GBSPLib.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "..\\" /I "SDKShare\Include" /I "..\..\MSDev60\Include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GBSPLIB_EXPORTS" /Fp"$(INTDIR)\GBSPLib.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\GBSPLib.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /dll /incremental:yes /pdb:"$(OUTDIR)\GBSPLib.pdb" /debug /machine:I386 /nodefaultlib /out:"$(OUTDIR)\GBSPLib.dll" /implib:"$(OUTDIR)\GBSPLib.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\Brush2.obj" \
	"$(INTDIR)\Bsp.obj" \
	"$(INTDIR)\Bsp2.obj" \
	"$(INTDIR)\Fill.obj" \
	"$(INTDIR)\Gbspfile.obj" \
	"$(INTDIR)\Gbsplib.obj" \
	"$(INTDIR)\Gbspprep.obj" \
	"$(INTDIR)\Leaf.obj" \
	"$(INTDIR)\Light.obj" \
	"$(INTDIR)\Map.obj" \
	"$(INTDIR)\Mathlib.obj" \
	"$(INTDIR)\Poly.obj" \
	"$(INTDIR)\Portals.obj" \
	"$(INTDIR)\PortFile.obj" \
	"$(INTDIR)\Rad.obj" \
	"$(INTDIR)\Texture.obj" \
	"$(INTDIR)\TJunct.obj" \
	"$(INTDIR)\Utils.obj" \
	"$(INTDIR)\Vis.obj" \
	"$(INTDIR)\Visflood.obj" \
	".\SDKShare\Lib\genesisd.lib" \
	"..\..\MSDev60\lib\Winspool.lib" \
	"..\..\MSDev60\lib\Comdlg32.lib" \
	"..\..\MSDev60\lib\Gdi32.lib" \
	"..\..\MSDev60\lib\Kernel32.lib" \
	"..\..\MSDev60\lib\Libcmtd.lib" \
	"..\..\MSDev60\lib\Oldnames.lib" \
	"..\..\MSDev60\lib\Shell32.lib" \
	"..\..\MSDev60\lib\User32.lib" \
	"..\..\MSDev60\lib\Uuid.lib" \
	"..\..\MSDev60\lib\Advapi32.lib"

"$(OUTDIR)\GBSPLib.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("GBSPLib.dep")
!INCLUDE "GBSPLib.dep"
!ELSE 
!MESSAGE Warning: cannot find "GBSPLib.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "GBSPLib - Win32 Release" || "$(CFG)" == "GBSPLib - Win32 Debug"
SOURCE=.\Brush2.cpp

"$(INTDIR)\Brush2.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Bsp.cpp

"$(INTDIR)\Bsp.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Bsp2.cpp

"$(INTDIR)\Bsp2.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Fill.Cpp

"$(INTDIR)\Fill.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Gbspfile.cpp

"$(INTDIR)\Gbspfile.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Gbsplib.cpp

"$(INTDIR)\Gbsplib.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Gbspprep.cpp

"$(INTDIR)\Gbspprep.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Leaf.cpp

"$(INTDIR)\Leaf.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Light.cpp

"$(INTDIR)\Light.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Map.cpp

"$(INTDIR)\Map.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Mathlib.cpp

"$(INTDIR)\Mathlib.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Poly.cpp

"$(INTDIR)\Poly.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Portals.cpp

"$(INTDIR)\Portals.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\PortFile.cpp

"$(INTDIR)\PortFile.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Rad.cpp

"$(INTDIR)\Rad.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Texture.cpp

"$(INTDIR)\Texture.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\TJunct.cpp

"$(INTDIR)\TJunct.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Utils.cpp

"$(INTDIR)\Utils.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Vis.cpp

"$(INTDIR)\Vis.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Visflood.cpp

"$(INTDIR)\Visflood.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

